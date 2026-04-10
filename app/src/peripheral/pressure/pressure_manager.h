#ifndef __PRESSURE_MANAGER_H__
#define __PRESSURE_MANAGER_H__

#include <stdbool.h>
#include <stdint.h>

#define PRESSURE_MANAGER_MAX_SENSORS 13U
#define PRESSURE_MANAGER_MAX_CAL_POINTS 5U

#ifndef PRESSURE_MANAGER_SENSOR_COUNT
#define PRESSURE_MANAGER_SENSOR_COUNT 1U
#endif

#ifndef PRESSURE_MANAGER_I2C_CLOCK
#define PRESSURE_MANAGER_I2C_CLOCK 100000UL
#endif

#ifndef PRESSURE_MANAGER_MPRLS_ADDR7
#define PRESSURE_MANAGER_MPRLS_ADDR7 0x18U
#endif

/*
 * MPRLS0300YG00001B 量程：0~300 mmHg(gauge)
 * 若后续更换同系列器件，可在这里调整量程换算范围。
 */
#ifndef PRESSURE_MANAGER_PRESSURE_MIN_MMHG
#define PRESSURE_MANAGER_PRESSURE_MIN_MMHG 0.0f
#endif

#ifndef PRESSURE_MANAGER_PRESSURE_MAX_MMHG
#define PRESSURE_MANAGER_PRESSURE_MAX_MMHG 300.0f
#endif

/*
 * MPRLS0300YG00001B uses transfer function B:
 * output counts span from 2.5% to 22.5% of the 24-bit ADC range.
 * Keep these values overrideable in case the sensor variant changes later.
 */
#ifndef PRESSURE_MANAGER_OUTPUT_MIN_COUNTS
#define PRESSURE_MANAGER_OUTPUT_MIN_COUNTS 0x066666UL
#endif

#ifndef PRESSURE_MANAGER_OUTPUT_MAX_COUNTS
#define PRESSURE_MANAGER_OUTPUT_MAX_COUNTS 0x399999UL
#endif

#ifndef PRESSURE_MANAGER_FILTER_WINDOW
#define PRESSURE_MANAGER_FILTER_WINDOW 16U
#endif

#ifndef PRESSURE_MANAGER_SENSOR_SAMPLE_HZ
#define PRESSURE_MANAGER_SENSOR_SAMPLE_HZ 10U
#endif

#ifndef PRESSURE_MANAGER_ZERO_CALIBRATION_SAMPLES
#define PRESSURE_MANAGER_ZERO_CALIBRATION_SAMPLES 32U
#endif

#ifndef PRESSURE_MANAGER_ZERO_START_DELAY_MS
#define PRESSURE_MANAGER_ZERO_START_DELAY_MS 2000U
#endif

#ifndef PRESSURE_MANAGER_ZERO_CAPTURE_THRESHOLD_MMHG
#define PRESSURE_MANAGER_ZERO_CAPTURE_THRESHOLD_MMHG 15.0f
#endif

#ifndef PRESSURE_MANAGER_ZERO_TRACK_THRESHOLD_MMHG
#define PRESSURE_MANAGER_ZERO_TRACK_THRESHOLD_MMHG 2.00f
#endif

#ifndef PRESSURE_MANAGER_ZERO_TRACK_SAMPLES
#define PRESSURE_MANAGER_ZERO_TRACK_SAMPLES 8U
#endif

#ifndef PRESSURE_MANAGER_ZERO_TRACK_SHIFT
#define PRESSURE_MANAGER_ZERO_TRACK_SHIFT 5U
#endif

#ifndef PRESSURE_MANAGER_ENABLE_ZERO_TRACKING
#define PRESSURE_MANAGER_ENABLE_ZERO_TRACKING 1U
#endif

typedef enum {
    PRESSURE_MANAGER_ERROR_NONE = 0,
    PRESSURE_MANAGER_ERROR_SELECT,
    PRESSURE_MANAGER_ERROR_I2C,
    PRESSURE_MANAGER_ERROR_BUSY_TIMEOUT,
    PRESSURE_MANAGER_ERROR_MEMORY,
    PRESSURE_MANAGER_ERROR_MATH,
    PRESSURE_MANAGER_ERROR_INVALID_INDEX,
} pressure_manager_error_t;

typedef enum {
    PRESSURE_MANAGER_ROUTE_DIRECT = 0,
    PRESSURE_MANAGER_ROUTE_TCA9548A,
} pressure_manager_route_type_t;

typedef struct {
    bool available;
    pressure_manager_route_type_t route_type;
    uint8_t mux_addr;
    uint8_t mux_channel;
    uint8_t sensor_addr;
} pressure_manager_route_info_t;

typedef struct {
    bool valid;
    int32_t reference_mmhg_x100;
    int32_t measured_mmhg_x100;
    uint32_t filtered_counts;
} pressure_manager_calibration_point_t;

typedef struct {
    uint8_t point_count;
    pressure_manager_calibration_point_t points[PRESSURE_MANAGER_MAX_CAL_POINTS];
} pressure_manager_calibration_t;

typedef struct {
    bool online;
    bool busy;
    bool data_valid;
    bool zero_calibrated;
    bool calibration_applied;
    uint8_t status;
    uint32_t sensor_raw_counts;
    uint32_t raw_counts;
    uint32_t filtered_counts;
    uint32_t zero_reference_counts;
    float measured_pressure_mmhg;
    float measured_pressure_kpa;
    float pressure_mmhg;
    float pressure_kpa;
    uint32_t update_tick;
    uint32_t error_count;
    uint8_t zero_sample_count;
    uint8_t calibration_point_count;
    pressure_manager_error_t last_error;
} pressure_manager_data_t;

void pressure_manager_init(void);
void pressure_manager_poll(void);
uint8_t pressure_manager_get_sensor_count(void);
bool pressure_manager_is_online(uint8_t index);
bool pressure_manager_get_data(uint8_t index, pressure_manager_data_t *data);
bool pressure_manager_get_route_info(uint8_t index, pressure_manager_route_info_t *route_info);
bool pressure_manager_get_calibration(uint8_t index, pressure_manager_calibration_t *calibration);
bool pressure_manager_set_calibration_point(uint8_t index,
                                            uint8_t point_index,
                                            int32_t reference_mmhg_x100,
                                            uint32_t filtered_counts);
bool pressure_manager_capture_calibration_point(uint8_t index,
                                                uint8_t point_index,
                                                int32_t reference_mmhg_x100);
bool pressure_manager_clear_calibration_point(uint8_t index, uint8_t point_index);
bool pressure_manager_clear_calibration(uint8_t index);
float pressure_manager_get_pressure_mmhg(uint8_t index);
float pressure_manager_get_pressure_kpa(uint8_t index);

/*
 * 可选钩子：
 * 1. 多路复用/片选时，重写 pressure_manager_select_sensor()
 * 2. 若单板上混挂不同地址，重写 pressure_manager_get_sensor_addr()
 */
bool pressure_manager_select_sensor(uint8_t index);
uint8_t pressure_manager_get_sensor_addr(uint8_t index);

#endif /* __PRESSURE_MANAGER_H__ */
