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
static uint32_t s_filter_counts[PRESSURE_MANAGER_SENSOR_COUNT][PRESSURE_MANAGER_FILTER_WINDOW];
static uint8_t s_filter_head[PRESSURE_MANAGER_SENSOR_COUNT];
static uint64_t s_zero_sum[PRESSURE_MANAGER_SENSOR_COUNT];
static uint8_t s_zero_track_hits[PRESSURE_MANAGER_SENSOR_COUNT];

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

static uint32_t pressure_manager_filter_raw_counts(uint8_t index, uint32_t raw_counts)
{
    uint32_t sorted_buf[PRESSURE_MANAGER_FILTER_WINDOW];
    pressure_manager_data_t *data = &s_pressure_data[index];
    uint32_t sample_count;
    uint32_t i;
    uint32_t j;
    uint32_t tmp;
    uint8_t head;

    head = s_filter_head[index];
    s_filter_counts[index][head] = raw_counts;
    head++;
    if(head >= PRESSURE_MANAGER_FILTER_WINDOW) {
        head = 0U;
    }
    s_filter_head[index] = head;

    if(data->zero_sample_count < PRESSURE_MANAGER_FILTER_WINDOW) {
        sample_count = (uint32_t)data->zero_sample_count + 1U;
    } else {
        sample_count = PRESSURE_MANAGER_FILTER_WINDOW;
    }

    for(i = 0U; i < sample_count; i++) {
        sorted_buf[i] = s_filter_counts[index][i];
    }

    for(i = 1U; i < sample_count; i++) {
        tmp = sorted_buf[i];
        j = i;
        while((j > 0U) && (sorted_buf[j - 1U] > tmp)) {
            sorted_buf[j] = sorted_buf[j - 1U];
            j--;
        }
        sorted_buf[j] = tmp;
    }

    return sorted_buf[sample_count / 2U];
}

static uint32_t pressure_manager_apply_zero_reference(uint32_t filtered_counts, uint32_t zero_reference_counts)
{
    int32_t compensated_counts;

    compensated_counts = (int32_t)filtered_counts;
    compensated_counts -= (int32_t)zero_reference_counts;
    compensated_counts += (int32_t)PRESSURE_MANAGER_OUTPUT_MIN_COUNTS;

    if(compensated_counts <= (int32_t)PRESSURE_MANAGER_OUTPUT_MIN_COUNTS) {
        return PRESSURE_MANAGER_OUTPUT_MIN_COUNTS;
    }

    if(compensated_counts >= (int32_t)PRESSURE_MANAGER_OUTPUT_MAX_COUNTS) {
        return PRESSURE_MANAGER_OUTPUT_MAX_COUNTS;
    }

    return (uint32_t)compensated_counts;
}

static void pressure_manager_track_zero_reference(uint8_t index,
                                                  uint32_t filtered_counts,
                                                  float pressure_mmhg)
{
    pressure_manager_data_t *data = &s_pressure_data[index];

#if (PRESSURE_MANAGER_ENABLE_ZERO_TRACKING == 0U)
    (void)index;
    (void)filtered_counts;
    (void)pressure_mmhg;
    (void)data;
    return;
#endif

    if(data->zero_calibrated == false) {
        return;
    }

    if((pressure_mmhg >= -PRESSURE_MANAGER_ZERO_TRACK_THRESHOLD_MMHG) &&
       (pressure_mmhg <= PRESSURE_MANAGER_ZERO_TRACK_THRESHOLD_MMHG)) {
        if(s_zero_track_hits[index] < 0xFFU) {
            s_zero_track_hits[index]++;
        }

        if(s_zero_track_hits[index] >= PRESSURE_MANAGER_ZERO_TRACK_SAMPLES) {
            data->zero_reference_counts =
                (((data->zero_reference_counts << PRESSURE_MANAGER_ZERO_TRACK_SHIFT) -
                  data->zero_reference_counts) + filtered_counts) >> PRESSURE_MANAGER_ZERO_TRACK_SHIFT;
        }
    } else {
        s_zero_track_hits[index] = 0U;
    }
}

static void pressure_manager_apply_measurement(uint8_t index, uint32_t raw_counts, uint32_t now)
{
    pressure_manager_data_t *data = &s_pressure_data[index];
    uint32_t filtered_counts;
    uint32_t compensated_counts;
    uint32_t zero_reference_counts;
    float pressure_mmhg;

    filtered_counts = pressure_manager_filter_raw_counts(index, raw_counts);

    data->sensor_raw_counts = raw_counts;
    data->filtered_counts = filtered_counts;
    data->busy = false;
    data->data_valid = true;
    data->update_tick = now;
    data->last_error = PRESSURE_MANAGER_ERROR_NONE;

    if(data->zero_calibrated == false) {
        s_zero_sum[index] += filtered_counts;
        if(data->zero_sample_count < PRESSURE_MANAGER_ZERO_CALIBRATION_SAMPLES) {
            data->zero_sample_count++;
        }

        data->zero_reference_counts =
            (uint32_t)(s_zero_sum[index] / (uint64_t)data->zero_sample_count);

        if(data->zero_sample_count >= PRESSURE_MANAGER_ZERO_CALIBRATION_SAMPLES) {
            data->zero_calibrated = true;
        }

        data->raw_counts = filtered_counts;
        data->pressure_mmhg = 0.0f;
        data->pressure_kpa = 0.0f;
        return;
    }

    zero_reference_counts = data->zero_reference_counts;
    compensated_counts = pressure_manager_apply_zero_reference(filtered_counts, zero_reference_counts);
    pressure_mmhg = pressure_manager_counts_to_mmhg(compensated_counts);

    pressure_manager_track_zero_reference(index, filtered_counts, pressure_mmhg);

    data->raw_counts = compensated_counts;
    data->pressure_mmhg = pressure_mmhg;
    data->pressure_kpa = data->pressure_mmhg * PRESSURE_MMHG_TO_KPA;
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
    pressure_manager_apply_measurement(s_active_index, raw_counts, now);

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
    memset(s_filter_counts, 0, sizeof(s_filter_counts));
    memset(s_filter_head, 0, sizeof(s_filter_head));
    memset(s_zero_sum, 0, sizeof(s_zero_sum));
    memset(s_zero_track_hits, 0, sizeof(s_zero_track_hits));

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
