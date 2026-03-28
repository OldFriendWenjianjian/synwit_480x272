/**************************************************************************/ /**
 * @file     dev_dac.c
 * @brief    dac相关配置函数
 * @version  V1.0
 * @date     2021.11.11
 ******************************************************************************/
#include "board.h"

uint8_t *wav_buf;
uint8_t wav_state;//bit0:0,未播放音频文件;1,正在播放音频文件
/**
  \brief   dac初始化
  \param [in]   无
  \return       无
  \note
 */
void dac_init(uint32_t format)
{
    DAC_Init(DAC, format);

    DAC_Open(DAC);
}

/**
  \brief   dac dma初始化
  \param [in]   addr    dac dma传输数据源地址
  \param [in]   count   传输 Unit 个数
  \return       无
  \note
 */
void dac_dma_init(uint32_t addr, uint32_t count, uint8_t unit, uint32_t sample_rate)
{
    DMA_InitStructure DMA_initStruct;

    DMA_initStruct.Mode = DMA_MODE_SINGLE;
    DMA_initStruct.Unit = unit;
    DMA_initStruct.Count = count;
    DMA_initStruct.SrcAddr = addr;
    DMA_initStruct.SrcAddrInc = 1;
    DMA_initStruct.DstAddr = (uint32_t)&DAC->DHR;
    DMA_initStruct.DstAddrInc = 0;
    DMA_initStruct.Priority = DMA_PRI_LOW;
    DMA_initStruct.INTEn = DMA_IT_DONE;
    DMA_initStruct.Handshake = DMA_EXHS_TIMR0;

    TIMR_Init(TIMR0, TIMR_MODE_TIMER, CyclesPerUs, 1000000 / sample_rate, 0);

    DMA_CH_Init(DMA_CH0, &DMA_initStruct);
}

/**
  \brief   dac反初始化
  \param [in]   无
  \return       无
  \note
 */
void dac_deinit(void)
{
    DMA_CH_Close(DMA_CH0);

    DAC_Close(DAC);
    TIMR_Stop(TIMR0);
}
