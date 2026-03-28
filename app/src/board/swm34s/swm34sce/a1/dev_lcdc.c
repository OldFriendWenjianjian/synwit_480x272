#include "SWM341.h"
#include "board.h"
#include "lvgl/lvgl.h"
#include "synwit_ui_framework/synwit_ui.h"

// 背光管脚
#define LCDBL_GPIO          GPIOA
#define LCDBL_PIN           PIN8

void lcd_backlight_off(void)
{
    GPIO_ClrBit(LCDBL_GPIO, LCDBL_PIN);
}

void lcd_backlight_on(void)
{
    GPIO_SetBit(LCDBL_GPIO, LCDBL_PIN);
}

static void rgb_init(void)
{
    /** 复位LCD模块 */
    GPIO_Init(GPIOD, PIN2, 1, 0, 0, 0);
    GPIO_SetBit(GPIOD, PIN2);
    /** 关闭背光 */
    GPIO_Init(LCDBL_GPIO, LCDBL_PIN, 1, 0, 0, 0);
    lcd_backlight_off();

	PORT_Init(PORTA, PIN1,  PORTA_PIN1_LCD_B3, 0);
	PORT_Init(PORTA, PIN2,  PORTA_PIN2_LCD_B4,  0);
	PORT_Init(PORTA, PIN9,  PORTA_PIN9_LCD_B5,  0);
	PORT_Init(PORTA, PIN10, PORTA_PIN10_LCD_B6, 0);
	PORT_Init(PORTA, PIN0,  PORTA_PIN0_LCD_B7, 0);
	
	PORT_Init(PORTA, PIN14, PORTA_PIN14_LCD_G2, 0);
	PORT_Init(PORTA, PIN15, PORTA_PIN15_LCD_G3, 0);
	PORT_Init(PORTC, PIN0,  PORTC_PIN0_LCD_G4,  0);
	PORT_Init(PORTC, PIN1,  PORTC_PIN1_LCD_G5,  0);
	PORT_Init(PORTC, PIN2,  PORTC_PIN2_LCD_G6,  0);
	PORT_Init(PORTC, PIN3,  PORTC_PIN3_LCD_G7,  0);
	
	PORT_Init(PORTN, PIN5,  PORTN_PIN5_LCD_R3,  0);
	PORT_Init(PORTD, PIN0,  PORTD_PIN0_LCD_R4, 0);
	PORT_Init(PORTD, PIN1,  PORTD_PIN1_LCD_R5, 0);	
	PORT_Init(PORTC, PIN12, PORTC_PIN12_LCD_R6, 0);
	PORT_Init(PORTC, PIN13, PORTC_PIN13_LCD_R7, 0);
	
	PORT_Init(PORTB, PIN2,  PORTB_PIN2_LCD_VSYNC, 0);
	PORT_Init(PORTM, PIN8,  PORTM_PIN8_LCD_HSYNC, 0);
	
	PORT_Init(PORTM, PIN11, PORTM_PIN11_LCD_DEN,   0);
	PORT_Init(PORTN, PIN0,  PORTN_PIN0_LCD_DCLK,  0);
}

void mpu_init(void)
{
    /** 复位LCD模块 */
    GPIO_Init(GPIOM, PIN1, 1, 1, 0, 0);
	// 此处请严格按照规格书所述的上电时序，否则无法正常显示
    GPIO_SetBit(GPIOM, PIN1);
    swm_delay_ms(10);
    GPIO_ClrBit(GPIOM, PIN1);
    swm_delay_ms(50);
    GPIO_SetBit(GPIOM, PIN1);
    swm_delay_ms(120);

    /** 关闭背光 */
    GPIO_Init(LCDBL_GPIO, LCDBL_PIN, 1, 1, 0, 0);
    lcd_backlight_off();

    // R- 5 bit : [ 3 ~ 7 ]

//	PORT_Init(PORTC, PIN4, PORTC_PIN4_LCD_R0, 0);
//	PORT_Init(PORTC, PIN5, PORTC_PIN5_LCD_R1, 0);
//	PORT_Init(PORTC, PIN8, PORTC_PIN8_LCD_R2, 0);
    PORT_Init(PORTN, PIN5,  PORTN_PIN5_LCD_R3, 0);
    PORT_Init(PORTD, PIN0, PORTD_PIN0_LCD_R4, 0);
    PORT_Init(PORTD, PIN1, PORTD_PIN1_LCD_R5, 0);
    PORT_Init(PORTC, PIN12, PORTC_PIN12_LCD_R6, 0);
    PORT_Init(PORTC, PIN13, PORTC_PIN13_LCD_R7, 0);

    // G- 6 bit : [ 2 ~ 7 ]

//	PORT_Init(PORTA, PIN12, PORTA_PIN12_LCD_G0, 0);
//	PORT_Init(PORTA, PIN13, PORTA_PIN13_LCD_G1, 0);
    PORT_Init(PORTA, PIN14, PORTA_PIN14_LCD_G2, 0);
    PORT_Init(PORTA, PIN15, PORTA_PIN15_LCD_G3, 0);
    PORT_Init(PORTC, PIN0, PORTC_PIN0_LCD_G4, 0);
    PORT_Init(PORTC, PIN1, PORTC_PIN1_LCD_G5, 0);
    PORT_Init(PORTC, PIN2, PORTC_PIN2_LCD_G6, 0);
    PORT_Init(PORTC, PIN3, PORTC_PIN3_LCD_G7, 0);

    // B- 5 bit : [ 3 ~ 7 ]

//	PORT_Init(PORTB, PIN1, PORTB_PIN1_LCD_B0, 0);
//	PORT_Init(PORTB, PIN11, PORTB_PIN11_LCD_B1, 0);
//	PORT_Init(PORTB, PIN13, PORTB_PIN13_LCD_B2, 0);
    PORT_Init(PORTA, PIN1, PORTA_PIN1_LCD_B3, 0);
    PORT_Init(PORTA, PIN2,  PORTA_PIN2_LCD_B4, 0);
    PORT_Init(PORTA, PIN9,  PORTA_PIN9_LCD_B5, 0);
    PORT_Init(PORTA, PIN10, PORTA_PIN10_LCD_B6, 0);
    PORT_Init(PORTA, PIN0, PORTA_PIN0_LCD_B7, 0);

    // CS / WR / RS / RD
	PORT_Init(PORTB, PIN2,  PORTB_PIN2_LCD_CS,  0);
	PORT_Init(PORTM, PIN8,  PORTM_PIN8_LCD_WR,  0);
	PORT_Init(PORTM, PIN11,  PORTM_PIN11_LCD_RS,  0);
	PORT_Init(PORTN, PIN0,  PORTN_PIN0_LCD_RD,  0);
}

/*
/---------------------------- LCD-SPI 串行接口 4line-------------------------//\
**
 * @brief   MPU-LCD 端口初始化
 * @param 	None
 * @retval	None
 */
void spi_4line_init(uint8_t WordSize)
{
    SPI_InitStructure SPI_initStruct;

	     /** 复位LCD模块 */
    GPIO_Init(LCD_GPIO_RST, LCD_PIN_RST, 1, 1, 0, 0);
	// 此处请严格按照规格书所述的上电时序，否则无法正常显示
    GPIO_SetBit(LCD_GPIO_RST, LCD_PIN_RST);
    swm_delay_ms(10);
    GPIO_ClrBit(LCD_GPIO_RST, LCD_PIN_RST);
    swm_delay_ms(50);
    GPIO_SetBit(LCD_GPIO_RST, LCD_PIN_RST);
    swm_delay_ms(120);

    /** 关闭背光 */
    GPIO_Init(LCDBL_GPIO, LCDBL_PIN, 1, 1, 0, 0);
    lcd_backlight_off();
	
//    extern void port_init_spi_8(void);
//    port_init_spi_8();
    
    GPIO_Init(LCD_GPIO_RS, LCD_PIN_RS, 1, 0, 0, 0);
    GPIO_Init(LCD_GPIO_CS, LCD_PIN_CS, 1, 0, 0, 0);
    GPIO_SetBit(LCD_GPIO_CS, LCD_PIN_CS); //初始拉高(便于SPI模式下使用)

    PORT_Init(LCD_SPI_PORT_SCLK, LCD_SPI_PIN_SCLK, LCD_SPI_FUNMUX_SCLK, 0);
    PORT_Init(LCD_SPI_PORT_MOSI, LCD_SPI_PIN_MOSI, LCD_SPI_FUNMUX_MOSI, 0);

    SPI_initStruct.clkDiv = SPI_CLKDIV_2; //标准 SPI 最高时钟 2 分频
    SPI_initStruct.FrameFormat = SPI_FORMAT_SPI;
    SPI_initStruct.SampleEdge = SPI_FIRST_EDGE;
    SPI_initStruct.IdleLevel = SPI_LOW_LEVEL;
    SPI_initStruct.WordSize = WordSize;
    SPI_initStruct.Master = 1;
    SPI_initStruct.RXThreshold = 0;
    SPI_initStruct.RXThresholdIEn = 0;
    SPI_initStruct.TXThreshold = 0;
    SPI_initStruct.TXThresholdIEn = 0;
    SPI_initStruct.TXCompleteIEn = 0;
    SPI_Init(LCD_SPI_X, &SPI_initStruct);
    SPI_Open(LCD_SPI_X);
}



/**
 * @brief   MPU-LCD 发送命令函数
 * @param 	data : 指令
 * @retval	None
 */
void lcd_write_cmd_spi_4line(uint8_t cmd)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    GPIO_AtomicClrBit(LCD_GPIO_RS, LCD_PIN_RS); //命令: 0  数据:1

    SPI_WriteWithWait(SPI1, cmd & 0xFF);

    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
}

/**
 * @brief   MPU-LCD 发送数据函数
 * @param 	data : 数据
 * @retval	None
 */
void lcd_write_data_spi_4line(uint16_t data)
{
    GPIO_AtomicClrBit(LCD_GPIO_CS, LCD_PIN_CS);
    GPIO_AtomicSetBit(LCD_GPIO_RS, LCD_PIN_RS); //命令: 0  数据:1

    SPI_WriteWithWait(SPI1, data);

    GPIO_AtomicSetBit(LCD_GPIO_CS, LCD_PIN_CS);
}

/**
 * @brief LCD控制器初始化
 * 
 */
void lcdc_init(void)
{
    const char *interface = synwit_ui_manifest_get_string("mcu.lcdc.interface", "spi_dma");
    
    if(strcmp(interface, "mpu") == 0) {
        lcdc_set_drive_mode(LCDC_DRIVE_MODE_MPU);
        mpu_init();
    } else if(strcmp(interface, "spi") == 0) {
        lcdc_set_drive_mode(LCDC_DRIVE_MODE_SPI);
        spi_4line_init(8);
    } else if(strcmp(interface, "spi_dma") == 0) {
        lcdc_set_drive_mode(LCDC_DRIVE_MODE_SPI_DMA);
        spi_4line_init(8);
    } else {
        lcdc_set_drive_mode(LCDC_DRIVE_MODE_RGB);
        rgb_init();
    }
}
