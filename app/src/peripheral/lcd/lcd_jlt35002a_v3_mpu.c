#include "SWM341.h"
#include "lvgl/lvgl.h"
#include "lcd/peripheral_lcd.h"
#include "lcd/ic_init/COG_ILI9488.c"

//volatile mpu_lcd_fun_t MPU_LCD;//实例

//----------------------------函数定义----------------------------//
static void mpu_lcd_init(void)
{
	MPU_LCD.init 	  = ILI9488_Init;
	MPU_LCD.setcur    = ILI9488_SetCursor;
	MPU_LCD.dp 		  = ILI9488_DrawPoint;
	MPU_LCD.clear 	  = ILI9488_Clear;
	MPU_LCD.dma_clear = ILI9488_DMA_Clear;

	MPU_LCD.init();//初始化
}

void lcd_jlt25002a_v3_init(int hres, int vres)
{
    MPULCD_InitStructure MPULCD_initStruct;
    
    LCD_HDOT = hres;
    LCD_VDOT = vres;
    
	MPULCD_initStruct.RDHoldTime = 8;
	MPULCD_initStruct.WRHoldTime = 3;
	MPULCD_initStruct.CSFall_WRFall = 2;
	MPULCD_initStruct.WRRise_CSRise = 1;
	MPULCD_initStruct.RDCSRise_Fall = 42;
	MPULCD_initStruct.WRCSRise_Fall = 15;
	MPULCD_Init(LCD, &MPULCD_initStruct);
    
    /* MPU屏初始化 */	
    mpu_lcd_init();
    MPU_LCD.clear(0xffff);
}

