#ifndef __DEV_I2C1_SENSOR_H__
#define __DEV_I2C1_SENSOR_H__

#include <stdbool.h>
#include <stdint.h>

#define DEV_I2C1_SENSOR_DEFAULT_CLOCK 100000UL

void dev_i2c1_sensor_init(uint32_t master_clk);
bool dev_i2c1_sensor_probe(uint8_t addr7);
bool dev_i2c1_sensor_write(uint8_t addr7, const uint8_t *data, uint32_t len);
bool dev_i2c1_sensor_read(uint8_t addr7, uint8_t *data, uint32_t len);
bool dev_i2c1_sensor_write_read(uint8_t addr7,
                                const uint8_t *tx_data,
                                uint32_t tx_len,
                                uint8_t *rx_data,
                                uint32_t rx_len);

#endif /* __DEV_I2C1_SENSOR_H__ */
