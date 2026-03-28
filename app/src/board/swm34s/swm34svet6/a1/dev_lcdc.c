#include "SWM341.h"
#include "board.h"
#include "lvgl/lvgl.h"
#include "synwit_ui_framework/synwit_ui.h"

#define LCDRST_GPIO GPIOD
#define LCDRST_PIN PIN1
#define LCDBL_GPIO GPIOA
#define LCDBL_PIN PIN1

void lcd_backlight_off(void)
{
    GPIO_ClrBit(LCDBL_GPIO, LCDBL_PIN);
}

void lcd_backlight_on(void)
{
    GPIO_SetBit(LCDBL_GPIO, LCDBL_PIN);
}

/**
 * @brief LCD控制器初始化
 * 
 */
void rgb_init(void)
{
    /** 复位LCD模块 */
    GPIO_Init(LCDRST_GPIO, LCDRST_PIN, 1, 0, 0, 0);
    // GPIO_ClrBit(LCDRST_GPIO, LCDRST_PIN);
    // for (uint32_t i = 0; i < CyclesPerUs; i++) __NOP();
    GPIO_SetBit(LCDRST_GPIO, LCDRST_PIN);
    /** 点亮背光 */
    GPIO_Init(LCDBL_GPIO, LCDBL_PIN, 1, 0, 0, 0);
    lcd_backlight_off();

#if  LV_COLOR_DEPTH == 32
    PORT_Init(PORTB, PIN1, PORTB_PIN1_LCD_B0, 0);
    PORT_Init(PORTB, PIN11, PORTB_PIN11_LCD_B1, 0);
    PORT_Init(PORTB, PIN13, PORTB_PIN13_LCD_B2, 0);
#endif
    PORT_Init(PORTB, PIN15, PORTB_PIN15_LCD_B3, 0);
    PORT_Init(PORTA, PIN2, PORTA_PIN2_LCD_B4, 0);
    PORT_Init(PORTA, PIN9, PORTA_PIN9_LCD_B5, 0);
    PORT_Init(PORTA, PIN10, PORTA_PIN10_LCD_B6, 0);
    PORT_Init(PORTA, PIN11, PORTA_PIN11_LCD_B7, 0);

#if  LV_COLOR_DEPTH == 32
    PORT_Init(PORTA, PIN12, PORTA_PIN12_LCD_G0, 0);
    PORT_Init(PORTA, PIN13, PORTA_PIN13_LCD_G1, 0);
#endif
    PORT_Init(PORTA, PIN14, PORTA_PIN14_LCD_G2, 0);
    PORT_Init(PORTA, PIN15, PORTA_PIN15_LCD_G3, 0);
    PORT_Init(PORTC, PIN0, PORTC_PIN0_LCD_G4, 0);
    PORT_Init(PORTC, PIN1, PORTC_PIN1_LCD_G5, 0);
    PORT_Init(PORTC, PIN2, PORTC_PIN2_LCD_G6, 0);
    PORT_Init(PORTC, PIN3, PORTC_PIN3_LCD_G7, 0);

#if  LV_COLOR_DEPTH == 32
    PORT_Init(PORTC, PIN4, PORTC_PIN4_LCD_R0, 0);
    PORT_Init(PORTC, PIN5, PORTC_PIN5_LCD_R1, 0);
    PORT_Init(PORTC, PIN8, PORTC_PIN8_LCD_R2, 0);
#endif
    PORT_Init(PORTC, PIN9, PORTC_PIN9_LCD_R3, 0);
    PORT_Init(PORTC, PIN10, PORTC_PIN10_LCD_R4, 0);
    PORT_Init(PORTC, PIN11, PORTC_PIN11_LCD_R5, 0);
    PORT_Init(PORTC, PIN12, PORTC_PIN12_LCD_R6, 0);
    PORT_Init(PORTC, PIN13, PORTC_PIN13_LCD_R7, 0);

    PORT_Init(PORTB, PIN2, PORTB_PIN2_LCD_VSYNC, 0);
#if 1
    PORT_Init(PORTB, PIN3, PORTB_PIN3_LCD_HSYNC, 0);
    PORT_Init(PORTB, PIN4, PORTB_PIN4_LCD_DEN, 0);
#else
    PORT_Init(PORTM, PIN8, PORTM_PIN8_LCD_HSYNC, 0);
    PORT_Init(PORTM, PIN11, PORTM_PIN11_LCD_DEN, 0);
#endif
    PORT_Init(PORTB, PIN5, PORTB_PIN5_LCD_DCLK, 0);
}

void mpu_init(void)
{
    /* TODO */
}

void spi_4line_init(uint8_t WordSize)
{
    /* TODO */
}

void lcd_write_cmd_spi_4line(uint8_t cmd)
{
    /* TODO */
}

void lcd_write_data_spi_4line(uint16_t data)
{
    /* TODO */
}

/**
 * @brief LCD控制器初始化
 * 
 */
void lcdc_init(void)
{
    const char *interface = synwit_ui_manifest_get_string("mcu.lcdc.interface", "sync");
    
    if(strcmp(interface, "mpu") == 0) {
        lcdc_set_drive_mode(LCDC_DRIVE_MODE_MPU);
        mpu_init();
    } else if(strcmp(interface, "spi") == 0) {
        lcdc_set_drive_mode(LCDC_DRIVE_MODE_SPI);
        spi_4line_init(8);
    }else {
        lcdc_set_drive_mode(LCDC_DRIVE_MODE_RGB);
        rgb_init();
    }
}