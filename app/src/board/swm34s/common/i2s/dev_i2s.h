/**************************************************************************/ /**
 * @file     dev_i2s.h
 * @brief    i2s相关配置函数
 * @version  V1.0
 * @date     2021.11.11
 ******************************************************************************/
#ifndef __DEV_I2S_H__
#define __DEV_I2S_H__
#include <stdint.h>

void i2s_init(uint8_t channels, uint8_t bps, uint32_t sample_rate);
void i2s_dma_init(void *data, uint32_t count);
void i2s_deinit(void);
uint8_t i2s_dma_done(void);

#endif //__DEV_I2S_H__
