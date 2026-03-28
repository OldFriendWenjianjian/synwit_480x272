#include "pressure_manager.h"

#include <string.h>

#include "board.h"

#if defined(SWM34SVET6_A3)
#include "swm34s/swm34svet6/a3/dev_i2c1_sensor.h"
#endif

#if PRESSURE_MANAGER_SENSOR_COUNT > PRESSURE_MANAGER_MAX_SENSORS
#error "PRESSURE_MANAGER_SENSOR_COUNT exceeds PRESSURE_MANAGER_MAX_SENSORS"
#endif

#define MPRLS_CMD_START_0          0xAAU
#define MPRLS_CMD_START_1          0x00U
#define MPRLS_CMD_START_2          0x00U
#define MPRLS_STATUS_POWERED       0x40U
#define MPRLS_STATUS_BUSY          0x20U
#define MPRLS_STATUS_MEMORY_ERROR  0x04U
#define MPRLS_STATUS_MATH_ERROR    0x01U
#define PRESSURE_MMHG_TO_KPA       0.133322368f
#define PRESSURE_CONVERT_DELAY_MS  5U
#define PRESSURE_RETRY_DELAY_MS    5U
#define PRESSURE_TIMEOUT_MS        30U
#define PRESSURE_OFFLINE_RETRY_MS  200U

typedef enum {
    PRESSURE_RUN_IDLE = 0,
    PRESSURE_RUN_WAIT_DATA,
} pressure_run_state_t;

static pressure_manager_data_t s_pressure_data[PRESSURE_MANAGER_SENSOR_COUNT];
static pressure_run_state_t s_run_state = PRESSURE_RUN_IDLE;
static uint8_t s_active_index = 0U;
static uint32_t s_next_action_tick = 0U;
static uint32_t s_convert_start_tick = 0U;

static bool pressure_manager_time_reached(uint32_t now, uint32_t target)
{
    return ((int32_t)(now - target) >= 0) ? true : false;
}

static float pressure_manager_counts_to_mmhg(uint32_t raw_counts)
{
    float span_counts = (float)(PRESSURE_MANAGER_OUTPUT_MAX_COUNTS - PRESSURE_MANAGER_OUTPUT_MIN_COUNTS);
    float pressure_span = PRESSURE_MANAGER_PRESSURE_MAX_MMHG - PRESSURE_MANAGER_PRESSURE_MIN_MMHG;
    float pressure;

    if(raw_counts <= PRESSURE_MANAGER_OUTPUT_MIN_COUNTS) {
        return PRESSURE_MANAGER_PRESSURE_MIN_MMHG;
    }

    if(raw_counts >= PRESSURE_MANAGER_OUTPUT_MAX_COUNTS) {
        return PRESSURE_MANAGER_PRESSURE_MAX_MMHG;
    }

    pressure = ((float)(raw_counts - PRESSURE_MANAGER_OUTPUT_MIN_COUNTS) * pressure_span) / span_counts;
    pressure += PRESSURE_MANAGER_PRESSURE_MIN_MMHG;

    return pressure;
}

static void pressure_manager_move_next(uint32_t now, uint32_t delay_ms)
{
    s_active_index++;
    if(s_active_index >= PRESSURE_MANAGER_SENSOR_COUNT) {
        s_active_index = 0U;
    }

    s_run_state = PRESSURE_RUN_IDLE;
    s_next_action_tick = now + delay_ms;
}

static void pressure_manager_set_error(uint8_t index,
                                       pressure_manager_error_t error,
                                       bool online,
                                       uint32_t now)
{
    pressure_manager_data_t *data = &s_pressure_data[index];

    data->online = online;
    data->busy = false;
    data->data_valid = false;
    data->update_tick = now;
    data->last_error = error;
    data->error_count++;
}

__attribute__((weak)) bool pressure_manager_select_sensor(uint8_t index)
{
    return (index == 0U);
}

__attribute__((weak)) uint8_t pressure_manager_get_sensor_addr(uint8_t index)
{
    (void)index;
    return PRESSURE_MANAGER_MPRLS_ADDR7;
}

#if defined(SWM34SVET6_A3)
static void pressure_manager_start_measurement(uint32_t now)
{
    static const uint8_t cmd[3] = {MPRLS_CMD_START_0, MPRLS_CMD_START_1, MPRLS_CMD_START_2};
    pressure_manager_data_t *data = &s_pressure_data[s_active_index];
    uint8_t addr = pressure_manager_get_sensor_addr(s_active_index);

    if(pressure_manager_select_sensor(s_active_index) == false) {
        pressure_manager_set_error(s_active_index, PRESSURE_MANAGER_ERROR_SELECT, false, now);
        pressure_manager_move_next(now, PRESSURE_OFFLINE_RETRY_MS);
        return;
    }

    if(dev_i2c1_sensor_write(addr, cmd, sizeof(cmd)) == false) {
        pressure_manager_set_error(s_active_index, PRESSURE_MANAGER_ERROR_I2C, false, now);
        pressure_manager_move_next(now, PRESSURE_OFFLINE_RETRY_MS);
        return;
    }

    data->busy = true;
    data->last_error = PRESSURE_MANAGER_ERROR_NONE;
    data->status = MPRLS_STATUS_BUSY;

    s_run_state = PRESSURE_RUN_WAIT_DATA;
    s_convert_start_tick = now;
    s_next_action_tick = now + PRESSURE_CONVERT_DELAY_MS;
}

static void pressure_manager_read_measurement(uint32_t now)
{
    pressure_manager_data_t *data = &s_pressure_data[s_active_index];
    uint8_t addr = pressure_manager_get_sensor_addr(s_active_index);
    uint8_t rx_buf[4];
    uint32_t raw_counts;

    if(pressure_manager_select_sensor(s_active_index) == false) {
        pressure_manager_set_error(s_active_index, PRESSURE_MANAGER_ERROR_SELECT, false, now);
        pressure_manager_move_next(now, PRESSURE_OFFLINE_RETRY_MS);
        return;
    }

    if(dev_i2c1_sensor_read(addr, rx_buf, sizeof(rx_buf)) == false) {
        pressure_manager_set_error(s_active_index, PRESSURE_MANAGER_ERROR_I2C, false, now);
        pressure_manager_move_next(now, PRESSURE_OFFLINE_RETRY_MS);
        return;
    }

    data->status = rx_buf[0];

    if((rx_buf[0] & MPRLS_STATUS_BUSY) != 0U) {
        if((now - s_convert_start_tick) >= PRESSURE_TIMEOUT_MS) {
            pressure_manager_set_error(s_active_index, PRESSURE_MANAGER_ERROR_BUSY_TIMEOUT, false, now);
            pressure_manager_move_next(now, PRESSURE_OFFLINE_RETRY_MS);
        } else {
            data->busy = true;
            s_next_action_tick = now + PRESSURE_RETRY_DELAY_MS;
        }
        return;
    }

    if((rx_buf[0] & MPRLS_STATUS_MEMORY_ERROR) != 0U) {
        pressure_manager_set_error(s_active_index, PRESSURE_MANAGER_ERROR_MEMORY, true, now);
        pressure_manager_move_next(now, PRESSURE_RETRY_DELAY_MS);
        return;
    }

    if((rx_buf[0] & MPRLS_STATUS_MATH_ERROR) != 0U) {
        pressure_manager_set_error(s_active_index, PRESSURE_MANAGER_ERROR_MATH, true, now);
        pressure_manager_move_next(now, PRESSURE_RETRY_DELAY_MS);
        return;
    }

    raw_counts = ((uint32_t)rx_buf[1] << 16)
               | ((uint32_t)rx_buf[2] << 8)
               | ((uint32_t)rx_buf[3] << 0);

    data->online = ((rx_buf[0] & MPRLS_STATUS_POWERED) != 0U) ? true : false;
    data->busy = false;
    data->data_valid = true;
    data->raw_counts = raw_counts;
    data->pressure_mmhg = pressure_manager_counts_to_mmhg(raw_counts);
    data->pressure_kpa = data->pressure_mmhg * PRESSURE_MMHG_TO_KPA;
    data->update_tick = now;
    data->last_error = PRESSURE_MANAGER_ERROR_NONE;

    pressure_manager_move_next(now, 0U);
}
#endif

void pressure_manager_init(void)
{
    memset(s_pressure_data, 0, sizeof(s_pressure_data));
    s_run_state = PRESSURE_RUN_IDLE;
    s_active_index = 0U;
    s_next_action_tick = 0U;
    s_convert_start_tick = 0U;

#if defined(SWM34SVET6_A3)
    dev_i2c1_sensor_init(PRESSURE_MANAGER_I2C_CLOCK);
#endif
}

void pressure_manager_poll(void)
{
    uint32_t now = swm_gettick();

    if(pressure_manager_time_reached(now, s_next_action_tick) == false) {
        return;
    }

#if defined(SWM34SVET6_A3)
    if(s_run_state == PRESSURE_RUN_IDLE) {
        pressure_manager_start_measurement(now);
    } else {
        pressure_manager_read_measurement(now);
    }
#else
    (void)now;
#endif
}

uint8_t pressure_manager_get_sensor_count(void)
{
    return (uint8_t)PRESSURE_MANAGER_SENSOR_COUNT;
}

bool pressure_manager_is_online(uint8_t index)
{
    if(index >= PRESSURE_MANAGER_SENSOR_COUNT) {
        return false;
    }

    return s_pressure_data[index].online;
}

bool pressure_manager_get_data(uint8_t index, pressure_manager_data_t *data)
{
    if((index >= PRESSURE_MANAGER_SENSOR_COUNT) || (data == 0)) {
        return false;
    }

    *data = s_pressure_data[index];
    return true;
}

float pressure_manager_get_pressure_mmhg(uint8_t index)
{
    if(index >= PRESSURE_MANAGER_SENSOR_COUNT) {
        return 0.0f;
    }

    return s_pressure_data[index].pressure_mmhg;
}

float pressure_manager_get_pressure_kpa(uint8_t index)
{
    if(index >= PRESSURE_MANAGER_SENSOR_COUNT) {
        return 0.0f;
    }

    return s_pressure_data[index].pressure_kpa;
}
