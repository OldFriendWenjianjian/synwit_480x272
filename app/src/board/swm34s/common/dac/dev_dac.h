/**************************************************************************/ /**
 * @file     dev_dac.h
 * @brief    dac相关配置函数
 * @version  V1.0
 * @date     2021.11.11
 ******************************************************************************/
#ifndef __DEV_DAC_H__
#define __DEV_DAC_H__
#include <stdint.h>

void dac_init(uint32_t format);
void dac_dma_init(uint32_t addr, uint32_t count, uint8_t unit, uint32_t sample_rate);
void dac_deinit(void);

#endif //__DEV_DAC_H__
