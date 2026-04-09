#ifndef __PRESSURE_MANAGER_H__
#define __PRESSURE_MANAGER_H__

#include <stdbool.h>
#include <stdint.h>

#define PRESSURE_MANAGER_MAX_SENSORS 13U

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
#define PRESSURE_MANAGER_FILTER_WINDOW 5U
#endif

#ifndef PRESSURE_MANAGER_ZERO_CALIBRATION_SAMPLES
#define PRESSURE_MANAGER_ZERO_CALIBRATION_SAMPLES 12U
#endif

#ifndef PRESSURE_MANAGER_ZERO_TRACK_THRESHOLD_MMHG
#define PRESSURE_MANAGER_ZERO_TRACK_THRESHOLD_MMHG 1.50f
#endif

#ifndef PRESSURE_MANAGER_ZERO_TRACK_SAMPLES
#define PRESSURE_MANAGER_ZERO_TRACK_SAMPLES 8U
#endif

#ifndef PRESSURE_MANAGER_ZERO_TRACK_SHIFT
#define PRESSURE_MANAGER_ZERO_TRACK_SHIFT 5U
#endif

#ifndef PRESSURE_MANAGER_ENABLE_ZERO_TRACKING
#define PRESSURE_MANAGER_ENABLE_ZERO_TRACKING 0U
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

typedef struct {
    bool online;
    bool busy;
    bool data_valid;
    bool zero_calibrated;
    uint8_t status;
    uint32_t sensor_raw_counts;
    uint32_t raw_counts;
    uint32_t filtered_counts;
    uint32_t zero_reference_counts;
    float pressure_mmhg;
    float pressure_kpa;
    uint32_t update_tick;
    uint32_t error_count;
    uint8_t zero_sample_count;
    pressure_manager_error_t last_error;
} pressure_manager_data_t;

void pressure_manager_init(void);
void pressure_manager_poll(void);
uint8_t pressure_manager_get_sensor_count(void);
bool pressure_manager_is_online(uint8_t index);
bool pressure_manager_get_data(uint8_t index, pressure_manager_data_t *data);
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
