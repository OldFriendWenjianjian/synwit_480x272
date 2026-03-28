/**************************************************************************/ /**
 * @file     dev_lcd_spi.c
 * @brief    lcd_spi相关配置函数
 * @version  V1.0
 * @date     2025.05.23
 ******************************************************************************/
#ifndef __DEV_LCD_SPI_H__
#define __DEV_LCD_SPI_H__

/*******************************************************************************************************************************************
 * static defines
 *******************************************************************************************************************************************/
#define LCD_GPIO_RST          GPIOM
#define LCD_PIN_RST           PIN1
 
#define LCD_GPIO_CS           GPIOB
#define LCD_PIN_CS            PIN2

/* LCD RS/DC: 数据或者命令管脚(1：数据读写, 0：命令读写) */
#define LCD_GPIO_RS           GPIOM
#define LCD_PIN_RS            PIN11

#define LCD_SPI_X             SPI1

/* Note: NO use MISO read, only use MOSI write. */
#define LCD_SPI_PORT_MOSI     PORTN
#define LCD_SPI_PIN_MOSI      PIN5
#define LCD_SPI_FUNMUX_MOSI   PORTN_PIN5_SPI1_MOSI

#define LCD_SPI_PORT_SCLK     PORTN
#define LCD_SPI_PIN_SCLK      PIN0
#define LCD_SPI_FUNMUX_SCLK   PORTN_PIN0_SPI1_SCLK

/** LCD_SPI TX DMA(only support SPI 8bit) */
#define LCD_SPI_TX_DMACHN     DMA_CH1
#define LCD_SPI_DMA_TX_TRIG   DMA_CH1##_##SPI1##TX
/* 341 的 DMA 单次传输最大支持 1M 个 Unit(没有 SDRAM 的情况下, 不可能会超出 unit 上限) */
#define DMA_SINGLE_CNT_MAX    ( 0x100000 )

#endif //__DEV_LCD_SPI_H__