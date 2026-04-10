#include "pressure_manager.h"

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "synwit_ui_framework/misc/preference.h"

#if defined(SWM34SVET6_A3)
#include "swm34s/swm34svet6/a3/dev_i2c1_sensor.h"
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
#define TCA9548A_ADDR0             0x70U
#define TCA9548A_ADDR1             0x71U
#define TCA9548A_CHANNEL_COUNT     8U
#define PRESSURE_MANAGER_PREF_FILE "S:spi:/pressure_calibration.pref"

typedef enum {
    PRESSURE_RUN_IDLE = 0,
    PRESSURE_RUN_WAIT_DATA,
} pressure_run_state_t;

typedef struct {
    bool available;
    pressure_manager_route_type_t route_type;
    uint8_t mux_addr;
    uint8_t mux_channel;
    uint8_t sensor_addr;
} pressure_manager_route_t;

typedef struct {
    int32_t x;
    int32_t y;
} pressure_manager_interp_point_t;

static pressure_manager_data_t s_pressure_data[PRESSURE_MANAGER_MAX_SENSORS];
static pressure_manager_calibration_t s_calibrations[PRESSURE_MANAGER_MAX_SENSORS];
static pressure_manager_route_t s_routes[PRESSURE_MANAGER_MAX_SENSORS];
static pressure_run_state_t s_run_state[PRESSURE_MANAGER_MAX_SENSORS];
static uint8_t s_sensor_count = 0U;
static uint8_t s_active_index = 0U;
static uint32_t s_next_action_tick[PRESSURE_MANAGER_MAX_SENSORS];
static uint32_t s_convert_start_tick[PRESSURE_MANAGER_MAX_SENSORS];
static uint32_t s_zero_start_tick = 0U;
static uint32_t s_filter_counts[PRESSURE_MANAGER_MAX_SENSORS][PRESSURE_MANAGER_FILTER_WINDOW];
static uint8_t s_filter_head[PRESSURE_MANAGER_MAX_SENSORS];
static uint8_t s_filter_sample_count[PRESSURE_MANAGER_MAX_SENSORS];
static uint64_t s_zero_sum[PRESSURE_MANAGER_MAX_SENSORS];
static uint8_t s_zero_track_hits[PRESSURE_MANAGER_MAX_SENSORS];
static bool s_mux_present[2];

static bool pressure_manager_time_reached(uint32_t now, uint32_t target)
{
    return ((int32_t)(now - target) >= 0) ? true : false;
}

static int32_t pressure_manager_float_to_x100(float value)
{
    if(value >= 0.0f) {
        return (int32_t)(value * 100.0f + 0.5f);
    }

    return (int32_t)(value * 100.0f - 0.5f);
}

static uint8_t pressure_manager_calibration_count_valid(const pressure_manager_calibration_t *calibration)
{
    uint8_t i;
    uint8_t count = 0U;

    for(i = 0U; i < PRESSURE_MANAGER_MAX_CAL_POINTS; i++) {
        if(calibration->points[i].valid) {
            count++;
        }
    }

    return count;
}

static void pressure_manager_update_calibration_meta(uint8_t index)
{
    s_calibrations[index].point_count = pressure_manager_calibration_count_valid(&s_calibrations[index]);
    s_pressure_data[index].calibration_point_count = s_calibrations[index].point_count;
    s_pressure_data[index].calibration_applied = (s_calibrations[index].point_count > 0U);
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
    uint32_t sample_count;
    uint32_t i;
    uint64_t sum;
    uint8_t head;

    head = s_filter_head[index];
    s_filter_counts[index][head] = raw_counts;
    head++;
    if(head >= PRESSURE_MANAGER_FILTER_WINDOW) {
        head = 0U;
    }
    s_filter_head[index] = head;

    if(s_filter_sample_count[index] < PRESSURE_MANAGER_FILTER_WINDOW) {
        s_filter_sample_count[index]++;
    }
    sample_count = s_filter_sample_count[index];

    sum = 0U;
    for(i = 0U; i < sample_count; i++) {
        sum += s_filter_counts[index][i];
    }

    return (sample_count == 0U) ? raw_counts : (uint32_t)(sum / sample_count);
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

static void pressure_manager_reset_zero_reference_capture(uint8_t index)
{
    pressure_manager_data_t *data = &s_pressure_data[index];

    s_zero_sum[index] = 0U;
    data->zero_sample_count = 0U;
    data->zero_reference_counts = 0U;
}

static bool pressure_manager_zero_reference_ready(uint32_t now)
{
    return pressure_manager_time_reached(now, s_zero_start_tick + PRESSURE_MANAGER_ZERO_START_DELAY_MS);
}

static bool pressure_manager_zero_reference_allowed(uint32_t filtered_counts)
{
    float raw_pressure_mmhg;

    raw_pressure_mmhg = pressure_manager_counts_to_mmhg(filtered_counts);
    return (raw_pressure_mmhg <= PRESSURE_MANAGER_ZERO_CAPTURE_THRESHOLD_MMHG) ? true : false;
}

static uint32_t pressure_manager_get_sensor_sample_period_ms(void)
{
    uint32_t sample_period_ms;

    if(PRESSURE_MANAGER_SENSOR_SAMPLE_HZ == 0U) {
        return 0U;
    }

    sample_period_ms = 1000U / PRESSURE_MANAGER_SENSOR_SAMPLE_HZ;
    if(sample_period_ms == 0U) {
        sample_period_ms = 1U;
    }

    return sample_period_ms;
}

static int32_t pressure_manager_get_calibration_zero_shift(uint8_t index)
{
    const pressure_manager_calibration_t *calibration = &s_calibrations[index];
    const pressure_manager_data_t *data = &s_pressure_data[index];
    uint8_t i;

    if((data->zero_calibrated == false) || (data->zero_reference_counts == 0U)) {
        return 0;
    }

    for(i = 0U; i < PRESSURE_MANAGER_MAX_CAL_POINTS; i++) {
        if((calibration->points[i].valid == false) ||
           (calibration->points[i].reference_mmhg_x100 != 0)) {
            continue;
        }

        return (int32_t)data->zero_reference_counts - (int32_t)calibration->points[i].filtered_counts;
    }

    return 0;
}

static uint8_t pressure_manager_collect_interp_points(uint8_t index,
                                                      pressure_manager_interp_point_t *points,
                                                      uint8_t max_points)
{
    int32_t zero_shift;
    uint8_t i;
    uint8_t count = 0U;
    const pressure_manager_calibration_t *calibration = &s_calibrations[index];

    zero_shift = pressure_manager_get_calibration_zero_shift(index);

    for(i = 0U; (i < PRESSURE_MANAGER_MAX_CAL_POINTS) && (count < max_points); i++) {
        if(calibration->points[i].valid == false) {
            continue;
        }

        points[count].x = (int32_t)calibration->points[i].filtered_counts + zero_shift;
        points[count].y = calibration->points[i].reference_mmhg_x100;
        count++;
    }

    for(i = 1U; i < count; i++) {
        pressure_manager_interp_point_t current = points[i];
        uint8_t j = i;

        while((j > 0U) && (points[j - 1U].x > current.x)) {
            points[j] = points[j - 1U];
            j--;
        }
        points[j] = current;
    }

    return count;
}

static float pressure_manager_apply_piecewise_calibration(uint8_t index,
                                                          uint32_t filtered_counts,
                                                          float measured_pressure_mmhg)
{
    pressure_manager_interp_point_t points[PRESSURE_MANAGER_MAX_CAL_POINTS];
    int32_t measured_x;
    int32_t result_x100;
    int64_t numerator;
    int32_t denominator;
    uint8_t count;
    uint8_t i;

    count = pressure_manager_collect_interp_points(index, points, PRESSURE_MANAGER_MAX_CAL_POINTS);
    if(count == 0U) {
        return measured_pressure_mmhg;
    }

    measured_x = (int32_t)filtered_counts;

    if(count == 1U) {
        result_x100 = points[0].y;
        return (float)result_x100 / 100.0f;
    }

    if(measured_x <= points[0].x) {
        i = 0U;
    } else if(measured_x >= points[count - 1U].x) {
        i = (uint8_t)(count - 2U);
    } else {
        for(i = 0U; i < (count - 1U); i++) {
            if((measured_x >= points[i].x) && (measured_x <= points[i + 1U].x)) {
                break;
            }
        }
    }

    denominator = points[i + 1U].x - points[i].x;
    if(denominator == 0) {
        result_x100 = points[i].y;
        return (float)result_x100 / 100.0f;
    }

    numerator = (int64_t)(measured_x - points[i].x) * (int64_t)(points[i + 1U].y - points[i].y);
    if(numerator >= 0) {
        numerator += (int64_t)(denominator / 2);
    } else {
        numerator -= (int64_t)(denominator / 2);
    }

    result_x100 = points[i].y + (int32_t)(numerator / denominator);
    return (float)result_x100 / 100.0f;
}

static uint8_t pressure_manager_mux_index_from_addr(uint8_t mux_addr)
{
    return (mux_addr == TCA9548A_ADDR1) ? 1U : 0U;
}

#if defined(SWM34SVET6_A3)
static bool pressure_manager_write_mux(uint8_t mux_addr, uint8_t value)
{
    if(s_mux_present[pressure_manager_mux_index_from_addr(mux_addr)] == false) {
        return false;
    }

    return dev_i2c1_sensor_write(mux_addr, &value, 1U);
}

static void pressure_manager_disable_all_mux_channels(void)
{
    if(s_mux_present[0]) {
        (void)pressure_manager_write_mux(TCA9548A_ADDR0, 0U);
    }
    if(s_mux_present[1]) {
        (void)pressure_manager_write_mux(TCA9548A_ADDR1, 0U);
    }
}

static bool pressure_manager_detect_direct_sensor(void)
{
    pressure_manager_disable_all_mux_channels();
    return dev_i2c1_sensor_probe(PRESSURE_MANAGER_MPRLS_ADDR7);
}

static bool pressure_manager_detect_mux_sensor(uint8_t mux_addr, uint8_t channel)
{
    uint8_t mask = (uint8_t)(1U << channel);

    pressure_manager_disable_all_mux_channels();
    if(pressure_manager_write_mux(mux_addr, mask) == false) {
        return false;
    }

    return dev_i2c1_sensor_probe(PRESSURE_MANAGER_MPRLS_ADDR7);
}

static void pressure_manager_add_route(pressure_manager_route_type_t route_type,
                                       uint8_t mux_addr,
                                       uint8_t mux_channel,
                                       uint8_t sensor_addr)
{
    pressure_manager_route_t *route;

    if(s_sensor_count >= PRESSURE_MANAGER_MAX_SENSORS) {
        return;
    }

    route = &s_routes[s_sensor_count];
    memset(route, 0, sizeof(*route));
    route->available = true;
    route->route_type = route_type;
    route->mux_addr = mux_addr;
    route->mux_channel = mux_channel;
    route->sensor_addr = sensor_addr;
    s_sensor_count++;
}

static void pressure_manager_detect_topology(void)
{
    uint8_t mux_addr;
    uint8_t channel;

    memset(s_routes, 0, sizeof(s_routes));
    s_sensor_count = 0U;
    s_mux_present[0] = dev_i2c1_sensor_probe(TCA9548A_ADDR0);
    s_mux_present[1] = dev_i2c1_sensor_probe(TCA9548A_ADDR1);

    if(s_mux_present[0] || s_mux_present[1]) {
        for(mux_addr = TCA9548A_ADDR0; mux_addr <= TCA9548A_ADDR1; mux_addr++) {
            if(s_mux_present[pressure_manager_mux_index_from_addr(mux_addr)] == false) {
                continue;
            }

            for(channel = 0U; channel < TCA9548A_CHANNEL_COUNT; channel++) {
                if(s_sensor_count >= PRESSURE_MANAGER_MAX_SENSORS) {
                    break;
                }

                if(pressure_manager_detect_mux_sensor(mux_addr, channel)) {
                    pressure_manager_add_route(PRESSURE_MANAGER_ROUTE_TCA9548A,
                                               mux_addr,
                                               channel,
                                               PRESSURE_MANAGER_MPRLS_ADDR7);
                }
            }
        }
    }

    pressure_manager_disable_all_mux_channels();

    if((s_sensor_count == 0U) && pressure_manager_detect_direct_sensor()) {
        pressure_manager_add_route(PRESSURE_MANAGER_ROUTE_DIRECT, 0U, 0U, PRESSURE_MANAGER_MPRLS_ADDR7);
    }
}
#else
static void pressure_manager_disable_all_mux_channels(void)
{
}

static void pressure_manager_detect_topology(void)
{
    memset(s_routes, 0, sizeof(s_routes));
    s_sensor_count = 0U;
    memset(s_mux_present, 0, sizeof(s_mux_present));
}
#endif

static void pressure_manager_route_key_from_route(const pressure_manager_route_t *route,
                                                  char *buffer,
                                                  uint32_t size)
{
    if((buffer == 0) || (size == 0U)) {
        return;
    }

    if(route->route_type == PRESSURE_MANAGER_ROUTE_DIRECT) {
        snprintf(buffer, size, "pressure.direct");
    } else {
        snprintf(buffer, size, "pressure.%02X.%u",
                 (unsigned int)route->mux_addr,
                 (unsigned int)route->mux_channel);
    }
}

static void pressure_manager_pref_remove_route(void *pref, const char *base_key)
{
    char key[64];
    uint8_t i;

    snprintf(key, sizeof(key), "%s.count", base_key);
    synwit_ui_pref_remove(pref, key);

    for(i = 0U; i < PRESSURE_MANAGER_MAX_CAL_POINTS; i++) {
        snprintf(key, sizeof(key), "%s.p%u.valid", base_key, (unsigned int)i);
        synwit_ui_pref_remove(pref, key);
        snprintf(key, sizeof(key), "%s.p%u.ref", base_key, (unsigned int)i);
        synwit_ui_pref_remove(pref, key);
        snprintf(key, sizeof(key), "%s.p%u.meas", base_key, (unsigned int)i);
        synwit_ui_pref_remove(pref, key);
        snprintf(key, sizeof(key), "%s.p%u.raw", base_key, (unsigned int)i);
        synwit_ui_pref_remove(pref, key);
        snprintf(key, sizeof(key), "%s.p%u.filt", base_key, (unsigned int)i);
        synwit_ui_pref_remove(pref, key);
    }
}

static void pressure_manager_pref_remove_all(void *pref)
{
    char base_key[32];
    uint8_t mux_addr;
    uint8_t channel;

    pressure_manager_pref_remove_route(pref, "pressure.direct");

    for(mux_addr = TCA9548A_ADDR0; mux_addr <= TCA9548A_ADDR1; mux_addr++) {
        for(channel = 0U; channel < TCA9548A_CHANNEL_COUNT; channel++) {
            snprintf(base_key, sizeof(base_key), "pressure.%02X.%u",
                     (unsigned int)mux_addr,
                     (unsigned int)channel);
            pressure_manager_pref_remove_route(pref, base_key);
        }
    }
}

static void pressure_manager_load_calibrations(void)
{
    void *pref;
    char base_key[32];
    char key[64];
    uint8_t index;
    uint8_t point_index;

    pref = synwit_ui_pref_open(PRESSURE_MANAGER_PREF_FILE, true, 0U);
    if(pref == 0) {
        return;
    }

    for(index = 0U; index < s_sensor_count; index++) {
        memset(&s_calibrations[index], 0, sizeof(s_calibrations[index]));
        pressure_manager_route_key_from_route(&s_routes[index], base_key, sizeof(base_key));

        for(point_index = 0U; point_index < PRESSURE_MANAGER_MAX_CAL_POINTS; point_index++) {
            snprintf(key, sizeof(key), "%s.p%u.valid", base_key, (unsigned int)point_index);
            s_calibrations[index].points[point_index].valid =
                (synwit_ui_pref_get_int(pref, key, 0) != 0) ? true : false;

            snprintf(key, sizeof(key), "%s.p%u.ref", base_key, (unsigned int)point_index);
            s_calibrations[index].points[point_index].reference_mmhg_x100 =
                synwit_ui_pref_get_int(pref, key, 0);

            snprintf(key, sizeof(key), "%s.p%u.meas", base_key, (unsigned int)point_index);
            s_calibrations[index].points[point_index].measured_mmhg_x100 =
                synwit_ui_pref_get_int(pref, key, 0);

            snprintf(key, sizeof(key), "%s.p%u.filt", base_key, (unsigned int)point_index);
            s_calibrations[index].points[point_index].filtered_counts =
                (uint32_t)synwit_ui_pref_get_int(pref, key, 0);
        }

        pressure_manager_update_calibration_meta(index);
    }

    synwit_ui_pref_close(pref);
}

static void pressure_manager_save_calibrations(void)
{
    void *pref;
    char base_key[32];
    char key[64];
    uint8_t index;
    uint8_t point_index;

    pref = synwit_ui_pref_open(PRESSURE_MANAGER_PREF_FILE, false, 0U);
    if(pref == 0) {
        return;
    }

    pressure_manager_pref_remove_all(pref);

    for(index = 0U; index < s_sensor_count; index++) {
        pressure_manager_route_key_from_route(&s_routes[index], base_key, sizeof(base_key));
        snprintf(key, sizeof(key), "%s.count", base_key);
        synwit_ui_pref_set_int(pref, key, s_calibrations[index].point_count);

        for(point_index = 0U; point_index < PRESSURE_MANAGER_MAX_CAL_POINTS; point_index++) {
            snprintf(key, sizeof(key), "%s.p%u.valid", base_key, (unsigned int)point_index);
            synwit_ui_pref_set_int(pref, key, s_calibrations[index].points[point_index].valid ? 1 : 0);
            snprintf(key, sizeof(key), "%s.p%u.ref", base_key, (unsigned int)point_index);
            synwit_ui_pref_set_int(pref, key, s_calibrations[index].points[point_index].reference_mmhg_x100);
            snprintf(key, sizeof(key), "%s.p%u.meas", base_key, (unsigned int)point_index);
            synwit_ui_pref_set_int(pref, key, s_calibrations[index].points[point_index].measured_mmhg_x100);
            snprintf(key, sizeof(key), "%s.p%u.filt", base_key, (unsigned int)point_index);
            synwit_ui_pref_set_int(pref, key, (int32_t)s_calibrations[index].points[point_index].filtered_counts);
        }
    }

    (void)synwit_ui_pref_save(pref, PRESSURE_MANAGER_PREF_FILE);
    synwit_ui_pref_close(pref);
}

static void pressure_manager_apply_measurement(uint8_t index, uint32_t raw_counts, uint32_t now)
{
    pressure_manager_data_t *data = &s_pressure_data[index];
    uint32_t filtered_counts;
    uint32_t compensated_counts;
    uint32_t zero_reference_counts;
    float measured_pressure_mmhg;

    filtered_counts = pressure_manager_filter_raw_counts(index, raw_counts);

    data->sensor_raw_counts = raw_counts;
    data->filtered_counts = filtered_counts;
    data->busy = false;
    data->data_valid = true;
    data->update_tick = now;
    data->last_error = PRESSURE_MANAGER_ERROR_NONE;

    if(data->zero_calibrated == false) {
        if((pressure_manager_zero_reference_ready(now) == false) ||
           (pressure_manager_zero_reference_allowed(filtered_counts) == false)) {
            pressure_manager_reset_zero_reference_capture(index);
            data->raw_counts = filtered_counts;
            data->measured_pressure_mmhg = 0.0f;
            data->measured_pressure_kpa = 0.0f;
            data->pressure_mmhg = 0.0f;
            data->pressure_kpa = 0.0f;
            pressure_manager_update_calibration_meta(index);
            return;
        }

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
        data->measured_pressure_mmhg = 0.0f;
        data->measured_pressure_kpa = 0.0f;
        data->pressure_mmhg = 0.0f;
        data->pressure_kpa = 0.0f;
        pressure_manager_update_calibration_meta(index);
        return;
    }

    zero_reference_counts = data->zero_reference_counts;
    compensated_counts = pressure_manager_apply_zero_reference(filtered_counts, zero_reference_counts);
    measured_pressure_mmhg = pressure_manager_counts_to_mmhg(compensated_counts);

    pressure_manager_track_zero_reference(index, filtered_counts, measured_pressure_mmhg);

    data->raw_counts = compensated_counts;
    data->measured_pressure_mmhg = measured_pressure_mmhg;
    data->measured_pressure_kpa = measured_pressure_mmhg * PRESSURE_MMHG_TO_KPA;
    pressure_manager_update_calibration_meta(index);
    data->pressure_mmhg = pressure_manager_apply_piecewise_calibration(index,
                                                                       filtered_counts,
                                                                       measured_pressure_mmhg);
    data->pressure_kpa = data->pressure_mmhg * PRESSURE_MMHG_TO_KPA;
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
#if defined(SWM34SVET6_A3)
    pressure_manager_route_t *route;
    uint8_t mask;

    if((index >= s_sensor_count) || (s_routes[index].available == false)) {
        return false;
    }

    pressure_manager_disable_all_mux_channels();
    route = &s_routes[index];
    if(route->route_type == PRESSURE_MANAGER_ROUTE_DIRECT) {
        return true;
    }

    mask = (uint8_t)(1U << route->mux_channel);
    return pressure_manager_write_mux(route->mux_addr, mask);
#else
    (void)index;
    return false;
#endif
}

__attribute__((weak)) uint8_t pressure_manager_get_sensor_addr(uint8_t index)
{
    if((index >= s_sensor_count) || (s_routes[index].available == false)) {
        return PRESSURE_MANAGER_MPRLS_ADDR7;
    }

    return s_routes[index].sensor_addr;
}

#if defined(SWM34SVET6_A3)
static void pressure_manager_schedule_next_sample(uint8_t index, uint32_t now)
{
    uint32_t target = s_convert_start_tick[index] + pressure_manager_get_sensor_sample_period_ms();

    if(pressure_manager_time_reached(now, target)) {
        target = now;
    }

    s_next_action_tick[index] = target;
}

static void pressure_manager_start_measurement(uint8_t index, uint32_t now)
{
    static const uint8_t cmd[3] = {MPRLS_CMD_START_0, MPRLS_CMD_START_1, MPRLS_CMD_START_2};
    pressure_manager_data_t *data = &s_pressure_data[index];
    uint8_t addr = pressure_manager_get_sensor_addr(index);

    if(pressure_manager_select_sensor(index) == false) {
        pressure_manager_set_error(index, PRESSURE_MANAGER_ERROR_SELECT, false, now);
        s_run_state[index] = PRESSURE_RUN_IDLE;
        s_next_action_tick[index] = now + PRESSURE_OFFLINE_RETRY_MS;
        return;
    }

    if(dev_i2c1_sensor_write(addr, cmd, sizeof(cmd)) == false) {
        pressure_manager_set_error(index, PRESSURE_MANAGER_ERROR_I2C, false, now);
        s_run_state[index] = PRESSURE_RUN_IDLE;
        s_next_action_tick[index] = now + PRESSURE_OFFLINE_RETRY_MS;
        return;
    }

    data->busy = true;
    data->last_error = PRESSURE_MANAGER_ERROR_NONE;
    data->status = MPRLS_STATUS_BUSY;

    s_run_state[index] = PRESSURE_RUN_WAIT_DATA;
    s_convert_start_tick[index] = now;
    s_next_action_tick[index] = now + PRESSURE_CONVERT_DELAY_MS;
}

static void pressure_manager_load_keil_calibrations(void)
{
    /* 第 4 个参数填写屏幕上显示的 Raw，即 5 次平均后的 filtered_counts。 */
    pressure_manager_set_calibration_point(0, 0, 0, 479033);
	pressure_manager_set_calibration_point(0, 1, 5550, 1096818);
	pressure_manager_set_calibration_point(0, 2, 15530, 2187785);
	pressure_manager_set_calibration_point(0, 3, 25200, 3294964);
	
    //pressure_manager_set_calibration_point(0, 1, 24700, 3237228);
    //pressure_manager_set_calibration_point(0, 2, 10180, 1523667);
    //pressure_manager_set_calibration_point(0, 3, 15200, 2084771);
    //pressure_manager_set_calibration_point(0, 4, 25140, 3192458);
}



static void pressure_manager_read_measurement(uint8_t index, uint32_t now)
{
    pressure_manager_data_t *data = &s_pressure_data[index];
    uint8_t addr = pressure_manager_get_sensor_addr(index);
    uint8_t rx_buf[4];
    uint32_t raw_counts;

    if(pressure_manager_select_sensor(index) == false) {
        pressure_manager_set_error(index, PRESSURE_MANAGER_ERROR_SELECT, false, now);
        s_run_state[index] = PRESSURE_RUN_IDLE;
        s_next_action_tick[index] = now + PRESSURE_OFFLINE_RETRY_MS;
        return;
    }

    if(dev_i2c1_sensor_read(addr, rx_buf, sizeof(rx_buf)) == false) {
        pressure_manager_set_error(index, PRESSURE_MANAGER_ERROR_I2C, false, now);
        s_run_state[index] = PRESSURE_RUN_IDLE;
        s_next_action_tick[index] = now + PRESSURE_OFFLINE_RETRY_MS;
        return;
    }

    data->status = rx_buf[0];

    if((rx_buf[0] & MPRLS_STATUS_BUSY) != 0U) {
        if((now - s_convert_start_tick[index]) >= PRESSURE_TIMEOUT_MS) {
            pressure_manager_set_error(index, PRESSURE_MANAGER_ERROR_BUSY_TIMEOUT, false, now);
            s_run_state[index] = PRESSURE_RUN_IDLE;
            s_next_action_tick[index] = now + PRESSURE_OFFLINE_RETRY_MS;
        } else {
            data->busy = true;
            s_run_state[index] = PRESSURE_RUN_WAIT_DATA;
            s_next_action_tick[index] = now + PRESSURE_RETRY_DELAY_MS;
        }
        return;
    }

    if((rx_buf[0] & MPRLS_STATUS_MEMORY_ERROR) != 0U) {
        pressure_manager_set_error(index, PRESSURE_MANAGER_ERROR_MEMORY, true, now);
        s_run_state[index] = PRESSURE_RUN_IDLE;
        s_next_action_tick[index] = now + PRESSURE_RETRY_DELAY_MS;
        return;
    }

    if((rx_buf[0] & MPRLS_STATUS_MATH_ERROR) != 0U) {
        pressure_manager_set_error(index, PRESSURE_MANAGER_ERROR_MATH, true, now);
        s_run_state[index] = PRESSURE_RUN_IDLE;
        s_next_action_tick[index] = now + PRESSURE_RETRY_DELAY_MS;
        return;
    }

    raw_counts = ((uint32_t)rx_buf[1] << 16)
               | ((uint32_t)rx_buf[2] << 8)
               | ((uint32_t)rx_buf[3] << 0);

    data->online = ((rx_buf[0] & MPRLS_STATUS_POWERED) != 0U) ? true : false;
    pressure_manager_apply_measurement(index, raw_counts, now);
    s_run_state[index] = PRESSURE_RUN_IDLE;
    pressure_manager_schedule_next_sample(index, now);
}
#endif

void pressure_manager_init(void)
{
    memset(s_pressure_data, 0, sizeof(s_pressure_data));
    memset(s_calibrations, 0, sizeof(s_calibrations));
    memset(s_routes, 0, sizeof(s_routes));
    memset(s_run_state, 0, sizeof(s_run_state));
    s_sensor_count = 0U;
    s_active_index = 0U;
    memset(s_next_action_tick, 0, sizeof(s_next_action_tick));
    memset(s_convert_start_tick, 0, sizeof(s_convert_start_tick));
    s_zero_start_tick = swm_gettick();
    memset(s_filter_counts, 0, sizeof(s_filter_counts));
    memset(s_filter_head, 0, sizeof(s_filter_head));
    memset(s_filter_sample_count, 0, sizeof(s_filter_sample_count));
    memset(s_zero_sum, 0, sizeof(s_zero_sum));
    memset(s_zero_track_hits, 0, sizeof(s_zero_track_hits));
    memset(s_mux_present, 0, sizeof(s_mux_present));

#if defined(SWM34SVET6_A3)
    dev_i2c1_sensor_init(PRESSURE_MANAGER_I2C_CLOCK);
#endif

    pressure_manager_detect_topology();
	pressure_manager_load_keil_calibrations();
    //pressure_manager_load_calibrations();
}

void pressure_manager_poll(void)
{
    uint32_t now = swm_gettick();
    uint8_t checked;
    uint8_t index;

    if(s_sensor_count == 0U) {
        return;
    }

#if defined(SWM34SVET6_A3)
    for(checked = 0U; checked < s_sensor_count; checked++) {
        index = (uint8_t)(s_active_index + checked);
        if(index >= s_sensor_count) {
            index = (uint8_t)(index - s_sensor_count);
        }

        if(pressure_manager_time_reached(now, s_next_action_tick[index]) == false) {
            continue;
        }

        if(s_run_state[index] == PRESSURE_RUN_IDLE) {
            pressure_manager_start_measurement(index, now);
        } else {
            pressure_manager_read_measurement(index, now);
        }

        s_active_index = (uint8_t)(index + 1U);
        if(s_active_index >= s_sensor_count) {
            s_active_index = 0U;
        }
        break;
    }
#else
    (void)now;
#endif
}

uint8_t pressure_manager_get_sensor_count(void)
{
    return s_sensor_count;
}

bool pressure_manager_is_online(uint8_t index)
{
    if(index >= s_sensor_count) {
        return false;
    }

    return s_pressure_data[index].online;
}

bool pressure_manager_get_data(uint8_t index, pressure_manager_data_t *data)
{
    if((index >= s_sensor_count) || (data == 0)) {
        return false;
    }

    *data = s_pressure_data[index];
    return true;
}

bool pressure_manager_get_route_info(uint8_t index, pressure_manager_route_info_t *route_info)
{
    if((index >= s_sensor_count) || (route_info == 0)) {
        return false;
    }

    memset(route_info, 0, sizeof(*route_info));
    route_info->available = s_routes[index].available;
    route_info->route_type = s_routes[index].route_type;
    route_info->mux_addr = s_routes[index].mux_addr;
    route_info->mux_channel = s_routes[index].mux_channel;
    route_info->sensor_addr = s_routes[index].sensor_addr;
    return true;
}

bool pressure_manager_get_calibration(uint8_t index, pressure_manager_calibration_t *calibration)
{
    if((index >= s_sensor_count) || (calibration == 0)) {
        return false;
    }

    *calibration = s_calibrations[index];
    return true;
}

bool pressure_manager_set_calibration_point(uint8_t index,
                                            uint8_t point_index,
                                            int32_t reference_mmhg_x100,
                                            uint32_t filtered_counts)
{
    pressure_manager_calibration_point_t *point;

    if((index >= s_sensor_count) || (point_index >= PRESSURE_MANAGER_MAX_CAL_POINTS)) {
        return false;
    }

    point = &s_calibrations[index].points[point_index];
    point->valid = true;
    point->reference_mmhg_x100 = reference_mmhg_x100;
    point->filtered_counts = filtered_counts;
    point->measured_mmhg_x100 = pressure_manager_float_to_x100(pressure_manager_counts_to_mmhg(filtered_counts));
    pressure_manager_update_calibration_meta(index);
    pressure_manager_save_calibrations();
    return true;
}

bool pressure_manager_capture_calibration_point(uint8_t index,
                                                uint8_t point_index,
                                                int32_t reference_mmhg_x100)
{
    if((index >= s_sensor_count) || (point_index >= PRESSURE_MANAGER_MAX_CAL_POINTS)) {
        return false;
    }

    if((s_pressure_data[index].data_valid == false) || (s_pressure_data[index].zero_calibrated == false)) {
        return false;
    }

    return pressure_manager_set_calibration_point(index,
                                                  point_index,
                                                  reference_mmhg_x100,
                                                  s_pressure_data[index].filtered_counts);
}

bool pressure_manager_clear_calibration_point(uint8_t index, uint8_t point_index)
{
    if((index >= s_sensor_count) || (point_index >= PRESSURE_MANAGER_MAX_CAL_POINTS)) {
        return false;
    }

    memset(&s_calibrations[index].points[point_index], 0, sizeof(s_calibrations[index].points[point_index]));
    pressure_manager_update_calibration_meta(index);
    pressure_manager_save_calibrations();
    return true;
}

bool pressure_manager_clear_calibration(uint8_t index)
{
    if(index >= s_sensor_count) {
        return false;
    }

    memset(&s_calibrations[index], 0, sizeof(s_calibrations[index]));
    pressure_manager_update_calibration_meta(index);
    pressure_manager_save_calibrations();
    return true;
}

float pressure_manager_get_pressure_mmhg(uint8_t index)
{
    if(index >= s_sensor_count) {
        return 0.0f;
    }

    return s_pressure_data[index].pressure_mmhg;
}

float pressure_manager_get_pressure_kpa(uint8_t index)
{
    if(index >= s_sensor_count) {
        return 0.0f;
    }

    return s_pressure_data[index].pressure_kpa;
}
