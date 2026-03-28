/**************************************************************************/ /**
 * @file     dev_i2s.c
 * @brief    i2s相关配置函数
 * @version  V1.0
 * @date     2021.11.11
 ******************************************************************************/
#include "board.h"

#define I2S_SPI_X         SPI1
#define I2S_DMA_CH_N      DMA_CH2
#define I2S_DMA_HS        DMA_CH2_SPI1TX

/**
 * @brief  I2S 播放音频初始化, 配置 DMA 传输
 * @param  channels    : 声音通道数
 * @param  bps         : 采样位深 /bit
 * @param  sample_rate : 采样频率 /Hz
 * @retval /
 */
void i2s_init(uint8_t channels, uint8_t bps, uint32_t sample_rate)
{
	PORT_Init(PORTC, PIN4, PORTC_PIN4_SPI1_SSEL, 0); // I2S_WS
	PORT_Init(PORTC, PIN7, PORTC_PIN7_SPI1_SCLK, 0); // I2S_CK
	PORT_Init(PORTC, PIN6, PORTC_PIN6_SPI1_MOSI, 0); // I2S_DO
    
	I2S_InitStructure I2S_initStruct;
	I2S_initStruct.Mode = I2S_MASTER_TX;
    //TM8211 描述： “输入采用LSBJ (Least Significant Bit Justified ) 格式, 数字编码格式采用MSB在前的补码格式”
    //即右对齐格式(like PT8211)
	I2S_initStruct.FrameFormat = I2S_MSB_JUSTIFIED;
    I2S_initStruct.ChannelLen = (bps <= 16) ? I2S_CHNNLEN_16 : I2S_CHNNLEN_32;
	I2S_initStruct.DataLen = (bps <= 8) ? I2S_DATALEN_8 : 
                             ((bps <= 16) ? I2S_DATALEN_16 : 
                             ((bps <= 24) ? I2S_DATALEN_24 : 
                             I2S_DATALEN_32));
	I2S_initStruct.ClkFreq = channels * bps * sample_rate;
	I2S_initStruct.RXThreshold = 0;
	I2S_initStruct.RXThresholdIEn = 0;
	I2S_initStruct.TXThreshold = 0;
	I2S_initStruct.TXThresholdIEn = 0;
	I2S_initStruct.TXCompleteIEn  = 0;
	I2S_Init(I2S_SPI_X, &I2S_initStruct);
	I2S_Open(I2S_SPI_X);
    
	// TX DMA
	I2S_SPI_X->CTRL |= (1 << SPI_CTRL_DMATXEN_Pos);

    DMA_InitStructure DMA_initStruct;
	DMA_initStruct.Mode = DMA_MODE_SINGLE;
	DMA_initStruct.Unit = DMA_UNIT_HALFWORD;
	DMA_initStruct.Count = 0;
	DMA_initStruct.SrcAddr = (uint32_t)0;
	DMA_initStruct.SrcAddrInc = 1;
//	DMA_initStruct.SrcAddrInc = 2; // Scatter-Gather 模式
	DMA_initStruct.DstAddr = (uint32_t)&I2S_SPI_X->DATA;
	DMA_initStruct.DstAddrInc = 0;
	DMA_initStruct.Handshake = I2S_DMA_HS;
	DMA_initStruct.Priority = DMA_PRI_HIGH;
    DMA_initStruct.INTEn = DMA_IT_DONE;
//	DMA_initStruct.INTEn = DMA_IT_SRCSG_HALF | DMA_IT_SRCSG_DONE;
	DMA_CH_Init(I2S_DMA_CH_N, &DMA_initStruct);
	//DMA_CH_Open(I2S_DMA_CH_N);
}

/**
 * @brief  I2S 播放音频, 重新配置 DMA 数据源地址和数量以实现连续播放
 * @param  data : 数据源
 * @param  cnt  : 传输 Unit 个数
 * @retval /
 */
void i2s_dma_init(void *data, uint32_t count)
{
    DMA_CH_SetSrcAddress(I2S_DMA_CH_N, (uint32_t)data);
    DMA_CH_SetCount(I2S_DMA_CH_N, count);
    DMA_CH_Open(I2S_DMA_CH_N);
}

void i2s_deinit(void)
{
    DMA_CH_Close(DMA_CH0);
	// TX DMA
	I2S_SPI_X->CTRL &= ~(SPI_CTRL_DMATXEN_Msk);
    I2S_Close(I2S_SPI_X);
}

/**
 * @brief  判断 DMA 是否传输完成
 * @param  /
 * @retval 0: 可以继续下一帧    other: 当前帧传输中
 */
uint8_t i2s_dma_done(void)
{
#if 1
    if (DMA_CH_INTStat(I2S_DMA_CH_N, DMA_IT_DONE))
    {
        DMA_CH_INTClr(I2S_DMA_CH_N, DMA_IT_DONE);
        return 0;
    }
#else 
	if (DMA_CH_INTStat(I2S_DMA_CH_N, DMA_IT_SRCSG_HALF))
	{
		DMA_CH_INTClr(I2S_DMA_CH_N, DMA_IT_SRCSG_HALF);
        return 0;
	}
	else if (DMA_CH_INTStat(I2S_DMA_CH_N, DMA_IT_SRCSG_DONE))
	{
		DMA_CH_INTClr(I2S_DMA_CH_N, DMA_IT_SRCSG_DONE);
        return 1;
	}
#endif
    return 2;
}
