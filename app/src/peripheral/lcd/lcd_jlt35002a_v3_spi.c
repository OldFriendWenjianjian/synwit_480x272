#include "SWM341.h"
#include "lvgl/lvgl.h"
#include "lcd/peripheral_lcd.h"
#include "lcd/ic_init/COG_ILI9488_SPI.c"

//volatile mpu_lcd_fun_t MPU_LCD;//实例

//----------------------------函数定义----------------------------//
static void mpu_lcd_init(void)
{
	MPU_LCD.init 	    = SPI_ILI9488_Init;
	MPU_LCD.setcur    = SPI_ILI9488_SetCursor;
	MPU_LCD.dp 		    = SPI_ILI9488_DrawPoint;
	MPU_LCD.clear 	  = SPI_ILI9488_Clear;
	MPU_LCD.dma_clear = SPI_ILI9488_DMA_Clear;
	
	MPU_LCD.init();//初始化
}

void lcd_jlt25002a_v3_spi_init(int hres, int vres)
{
    mpu_lcd_init();
	MPU_LCD.clear(0xF100);
}

