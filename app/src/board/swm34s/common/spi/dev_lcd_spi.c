/**************************************************************************/ /**
 * @file     dev_lcd_spi.c
 * @brief    lcd_spi相关配置函数
 * @version  V1.0
 * @date     2025.05.23
 ******************************************************************************/
#include "SWM341.h"
#include "dev_lcd_spi.h"

#define __DECLARE_LCD_MPU_FUNC_NAME(__base_class_name, __interface_name)      __base_class_name##_##__interface_name

#if 1 /* 调试打印 */
#include <stdio.h>
#define LCD_LOG(...)             printf(__VA_ARGS__)
#else
#define LCD_LOG(...)
#endif

static void spi_set_unit_size(uint8_t unit_bit)
{
    if (unit_bit >= 4 && unit_bit <= 16) {
        //SPI_Close(LCD_SPI_X);
        /* 重新配置 SPI 传输单位大小(4 ~ 16 bit) */
        LCD_SPI_X->CTRL &= ~SPI_CTRL_SIZE_Msk;
        LCD_SPI_X->CTRL |= ((unit_bit - 1) << SPI_CTRL_SIZE_Pos);
        //SPI_Open(LCD_SPI_X);
    } else {
        LCD_LOG("[%s] param [%d].\r\n", __FUNCTION__, unit_bit);
        for (;;) __NOP();
    }        
}

#define spi_dma_xfer_color(data, pixels)            spi_dma_xfer(data, pixels, 0, 0)
#define spi_dma_xfer_bitmap(data, pixels)           spi_dma_xfer(data, pixels, 0, 1)
#define spi_dma_xfer_bitmap_async(data, pixels)     spi_dma_xfer(data, pixels, 1, 1)

static uint8_t spi_dma_xfer(void *data, uint32_t pixels, uint8_t async, uint8_t src_inc)
{
    /* SPI TX DMA : 通过 DMA 写 SPI-FIFO */
    LCD_SPI_X->CTRL &= ~(SPI_CTRL_DMARXEN_Msk | SPI_CTRL_DMATXEN_Msk);
    LCD_SPI_X->CTRL |= SPI_CTRL_DMATXEN_Msk;

    DMA_InitStructure DMA_initStruct;
    DMA_initStruct.Mode = DMA_MODE_SINGLE;
    DMA_initStruct.Unit = DMA_UNIT_HALFWORD; /* RGB565 */
    DMA_initStruct.Count = DMA_SINGLE_CNT_MAX;
    DMA_initStruct.SrcAddr = (uint32_t)data;
    DMA_initStruct.SrcAddrInc = (src_inc) ? 1 : 0;
    DMA_initStruct.DstAddr = (uint32_t)&LCD_SPI_X->DATA;
    DMA_initStruct.DstAddrInc = 0;
    DMA_initStruct.Handshake = LCD_SPI_DMA_TX_TRIG;
    DMA_initStruct.Priority = DMA_PRI_LOW;
    DMA_initStruct.INTEn = 0; /* 失能中断 */
    DMA_CH_Init(LCD_SPI_TX_DMACHN, &DMA_initStruct);

    /* 注意: 当 SPI-DMA 工作期间, 不能使用 SPI 来发送其他控制指令, 必须等待 DMA 传输完成才能发起新的数据/指令传输 */
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    GPIO_AtomicSetBit(LCD_GPIO_RS, LCD_PIN_RS); /* Command: 0    Data:1 */

    /* DMA Sync */
    uint32_t data_addr = (uint32_t)data;
    while (pixels > DMA_SINGLE_CNT_MAX)
    {
        uint32_t single_count = (pixels > DMA_SINGLE_CNT_MAX) ? DMA_SINGLE_CNT_MAX : pixels;

        DMA_CH_SetCount(LCD_SPI_TX_DMACHN, single_count);
        DMA_CH_SetSrcAddress(LCD_SPI_TX_DMACHN, data_addr);

        DMA_CH_Open(LCD_SPI_TX_DMACHN);

        while (0 == DMA_CH_INTStat(LCD_SPI_TX_DMACHN, DMA_IT_DONE)) __NOP();
        DMA_CH_INTClr(LCD_SPI_TX_DMACHN, DMA_IT_DONE);

        pixels -= single_count;
        data_addr += (src_inc) ? (single_count << 1) : 0;
        
        /* wait SPI no busy */
        while (LCD_SPI_X->STAT & SPI_STAT_BUSY_Msk) __NOP();
    }
    /* DMA last xfer */
    DMA_CH_SetCount(LCD_SPI_TX_DMACHN, pixels);
    DMA_CH_SetSrcAddress(LCD_SPI_TX_DMACHN, data_addr);
    if (0 != async) {
        NVIC_EnableIRQ(DMA_IRQn); /* DMA_CH_INTEn() 内部实现未操作 NVIC, 须搭配 NVIC_EnableIRQ 使用 */
        DMA_CH_INTEn(LCD_SPI_TX_DMACHN, DMA_IT_DONE);
    }
    DMA_CH_Open(LCD_SPI_TX_DMACHN);
    
    if (0 == async) {
        while (0 == DMA_CH_INTStat(LCD_SPI_TX_DMACHN, DMA_IT_DONE)) __NOP();
        DMA_CH_INTClr(LCD_SPI_TX_DMACHN, DMA_IT_DONE);

        /* wait SPI no busy */
        while (LCD_SPI_X->STAT & SPI_STAT_BUSY_Msk) __NOP();

        GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
        /* 释放 DMA 写 SPI-FIFO */
        LCD_SPI_X->CTRL &= ~SPI_CTRL_DMATXEN_Msk;
    }
    return 0;
}
/*******************************************************************************************************************************************
 * public functions
 *******************************************************************************************************************************************/
void __DECLARE_LCD_MPU_FUNC_NAME(port_init, spi_9)(void)
{
    GPIO_INIT(LCD_GPIO_CS, LCD_PIN_CS, GPIO_OUTPUT);
    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);

    PORT_Init(LCD_SPI_PORT_SCLK, LCD_SPI_PIN_SCLK, LCD_SPI_FUNMUX_SCLK, 0);
    PORT_Init(LCD_SPI_PORT_MOSI, LCD_SPI_PIN_MOSI, LCD_SPI_FUNMUX_MOSI, 0);

    SPI_InitStructure SPI_initStruct;
    /* note: 
    * 对于 SPI 驱动显示屏, 常见的 ILI9488 / ST7789 规格书时序约束中描述 DBI Type C Option 3 (4-Line SPI System) Timing Characteristics
    * SCL T-scycw => Serial clock cycle (Write command & data ram) = 66ns => 即 15.15MHz 左右.
    * (通常留有一定余量, 但跟显示模组厂商的集成设计也存在关联, 请咨询显示模组厂商.)
    * SWM 芯片的 SPI 模块最高速率可达系统时钟 2 分频, 对于 341 即 150M / 2 = 75M, 故实际应用频率是否超频交由用户自行评估设定.
    */
    SPI_initStruct.clkDiv = SPI_CLKDIV_2; /* 标准 SPI 模式下最高时钟可设为系统主频的 2 分频 */
    SPI_initStruct.FrameFormat = SPI_FORMAT_SPI;
    SPI_initStruct.SampleEdge = SPI_FIRST_EDGE;
    SPI_initStruct.IdleLevel = SPI_LOW_LEVEL;
    SPI_initStruct.WordSize = 9;
    SPI_initStruct.Master = 1;
    SPI_initStruct.RXThreshold = 0;
    SPI_initStruct.RXThresholdIEn = 0;
    SPI_initStruct.TXThreshold = 0;
    SPI_initStruct.TXThresholdIEn = 0;
    SPI_initStruct.TXCompleteIEn = 0;
    SPI_Init(LCD_SPI_X, &SPI_initStruct);
    SPI_Open(LCD_SPI_X);
}
void __DECLARE_LCD_MPU_FUNC_NAME(port_init, spi_8)(void)
{
    __DECLARE_LCD_MPU_FUNC_NAME(port_init, spi_9)();

    spi_set_unit_size(8); //keep default 8bit
    
    GPIO_INIT(LCD_GPIO_RS, LCD_PIN_RS, GPIO_OUTPUT);
    GPIO_AtomicSetBit(LCD_GPIO_RS, LCD_PIN_RS);
}

void __DECLARE_LCD_MPU_FUNC_NAME(port_deinit, spi_9)(void)
{
    /* 将已使用的端口映射为普通 GPIO 功能, 配置为输入 & 无上拉 & 无下拉, 并关闭数字输入使能. */
}
void __DECLARE_LCD_MPU_FUNC_NAME(port_deinit, spi_8)(void)
{
}

/**
 * @brief  画点
 * @param  c : 色彩(only support RGB565)
 * @retval \
 */
void __DECLARE_LCD_MPU_FUNC_NAME(draw_point, spi_9)(uint32_t c)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);

    SPI_WriteWithWait(LCD_SPI_X, (((c >> 8) & 0xFF) | (1 << 8)));
    SPI_WriteWithWait(LCD_SPI_X, (((c >> 0) & 0xFF) | (1 << 8)));

    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
}
void __DECLARE_LCD_MPU_FUNC_NAME(draw_point, spi_8)(uint32_t c)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    GPIO_AtomicSetBit(LCD_GPIO_RS, LCD_PIN_RS); /* Command: 0    Data:1 */

    SPI_WriteWithWait(LCD_SPI_X, (c >> 8) & 0xFF);
    SPI_WriteWithWait(LCD_SPI_X, (c >> 0) & 0xFF);

    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
}

/**
 * @brief  填充纯色
 * @param  color  : 色彩(only support RGB565)
 * @param  pixels : 像素个数
 * @retval \
 */
void __DECLARE_LCD_MPU_FUNC_NAME(fill_color, spi_9)(uint32_t color, uint32_t pixels)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    for (uint32_t i = 0; i < pixels; ++i)
    {
        SPI_WriteWithWait(LCD_SPI_X, ((color >> 8) & 0xFF) | (1 << 8));
        SPI_WriteWithWait(LCD_SPI_X, ((color >> 0) & 0xFF) | (1 << 8));
    }
    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
}
void __DECLARE_LCD_MPU_FUNC_NAME(fill_color, spi_8)(uint32_t color, uint32_t pixels)
{
    /* 配置 SPI 传输单位大小(16 bit) */
    spi_set_unit_size(16);

    if (0 != spi_dma_xfer_color(&color, pixels))
    {
        GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
        GPIO_AtomicSetBit(LCD_GPIO_RS, LCD_PIN_RS); /* Command: 0    Data:1 */
        for (uint32_t i = 0; i < pixels; ++i)
        {
            SPI_WriteWithWait(LCD_SPI_X, color);
        }
        GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
    }

    /* 如果 SPI 传输单位大小有变化则恢复为默认 8 bit */
    spi_set_unit_size(8);
}

/**
 * @brief  绘图
 * @param  data   : 图像数据源(only support RGB565, 16bit alignment)
 * @param  pixels : 像素个数
 * @retval \
 */
void __DECLARE_LCD_MPU_FUNC_NAME(flush_bitmap, spi_9)(void *data, uint32_t pixels)
{
    if (!(data && 0 == ((uint32_t)data & 0x01) && pixels)) {
        LCD_LOG("[%s] param [%p][%d].\r\n", __FUNCTION__, data, pixels);
        return;
    }
    volatile uint16_t *p = (volatile uint16_t *)data;
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    for (uint32_t i = 0; i < pixels; ++i)
    {
        SPI_WriteWithWait(LCD_SPI_X, ((p[i] >> 8) & 0xFF) | (1 << 8));
        SPI_WriteWithWait(LCD_SPI_X, ((p[i] >> 0) & 0xFF) | (1 << 8));
    }
    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
}
void __DECLARE_LCD_MPU_FUNC_NAME(flush_bitmap, spi_8)(void *data, uint32_t pixels)
{
    if (!(data && 0 == ((uint32_t)data & 0x01) && pixels)) {
        LCD_LOG("[%s] param [%p][%d].\r\n", __FUNCTION__, data, pixels);
        return;
    }
    /* 配置 SPI 传输单位大小(16 bit) */
    spi_set_unit_size(16);
    
    if (0 != spi_dma_xfer_bitmap(data, pixels))
    {
        volatile uint16_t *p = (volatile uint16_t *)data;
        GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
        GPIO_AtomicSetBit(LCD_GPIO_RS, LCD_PIN_RS); /* Command: 0    Data:1 */
        for (uint32_t i = 0; i < pixels; ++i)
        {
            SPI_WriteWithWait(LCD_SPI_X, p[i]);
        }
        GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
    }
    
    /* 如果 SPI 传输单位大小有变化则恢复为默认 8 bit */
    spi_set_unit_size(8);
}

/*******************************************************************************************************************************************
 * public extra functions
 *******************************************************************************************************************************************/
/**
 * @brief  异步绘图
 * @param  data : 数据源(only support RGB565, 16bit alignment)
 * @param  pixels  : 像素个数
 * @retval 0: async vaild   other: async invaild
 */
uint8_t __DECLARE_LCD_MPU_FUNC_NAME(flush_bitmap_async, spi_8)(void *data, uint32_t pixels)
{
    if (!(data && 0 == ((uint32_t)data & 0x01) && pixels)) {
        LCD_LOG("[%s] param [%p][%d].\r\n", __FUNCTION__, data, pixels);
        return 1;
    }
    /* 配置 SPI 传输单位大小(16 bit) */
    spi_set_unit_size(16);

    if (0 != spi_dma_xfer_bitmap_async(data, pixels))
    {
        spi_set_unit_size(8);
        return 2;
    }
    return 0;
}

/**
 * @brief  异步画图完成就绪回调
 * @param  \
 * @retval 0: transfer completed   other: transfer busy
 */
uint8_t __DECLARE_LCD_MPU_FUNC_NAME(flush_bitmap_async_done, spi_8)(void)
{
    if (DMA_CH_INTStat(LCD_SPI_TX_DMACHN, DMA_IT_DONE))
    {
        DMA_CH_INTClr(LCD_SPI_TX_DMACHN, DMA_IT_DONE);

        /* wait SPI no busy */
        while (LCD_SPI_X->STAT & SPI_STAT_BUSY_Msk) __NOP();
        
        GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
        
        /* 释放 DMA 写 SPI-FIFO */
        LCD_SPI_X->CTRL &= ~SPI_CTRL_DMATXEN_Msk;

        /* 如果 SPI 传输单位大小有变化则恢复为默认 8 bit
        if ( (LCD_SPI_X->CTRL & SPI_CTRL_SIZE_Msk) != ( (8 - 1) << SPI_CTRL_SIZE_Pos ) )
        */
        {
            spi_set_unit_size(8);
        }
        return 0;
    }
    return 1;
}

/*******************************************************************************************************************************************
 * public base functions
 *******************************************************************************************************************************************/
void __DECLARE_LCD_MPU_FUNC_NAME(write_cmd, spi_8)(uint32_t val)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    GPIO_AtomicClrBit(LCD_GPIO_RS, LCD_PIN_RS); /* Command: 0    Data:1 */

    SPI_WriteWithWait(LCD_SPI_X, val);

    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
}

void __DECLARE_LCD_MPU_FUNC_NAME(write_cmd, spi_9)(uint32_t val)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    val &= 0xFF; /* Command: 0    Data:1 */

    SPI_WriteWithWait(LCD_SPI_X, val);

    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
}

void __DECLARE_LCD_MPU_FUNC_NAME(write_data, spi_8)(uint32_t val)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    GPIO_AtomicSetBit(LCD_GPIO_RS, LCD_PIN_RS); /* Command: 0    Data:1 */

    SPI_WriteWithWait(LCD_SPI_X, val);

    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
}

void __DECLARE_LCD_MPU_FUNC_NAME(write_data, spi_9)(uint32_t val)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    val |= (1 << 8); /* Command: 0    Data:1 */

    SPI_WriteWithWait(LCD_SPI_X, val);

    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
}

uint32_t __DECLARE_LCD_MPU_FUNC_NAME(read_data, spi_8)(void)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    GPIO_AtomicSetBit(LCD_GPIO_RS, LCD_PIN_RS); /* Command: 0    Data:1 */
    
    uint32_t data = SPI_ReadWrite(LCD_SPI_X, 0xFF);

    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
    return data;
}

uint32_t __DECLARE_LCD_MPU_FUNC_NAME(read_data, spi_9)(void)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    
    uint32_t data = SPI_ReadWrite(LCD_SPI_X, 0xFF | (1 << 8)); /* Command: 0    Data:1 */

    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
    return data;
}
