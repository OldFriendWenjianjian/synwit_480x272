#include "SWM341.h"
#include "lvgl/lvgl.h"
#include "lcd/peripheral_lcd.h"
#include "lcd/ic_init/COG_GC9307N.c"

//volatile mpu_lcd_fun_t MPU_LCD;//实例

//----------------------------函数定义----------------------------//
static void mpu_lcd_init(void)
{
	MPU_LCD.init 	  = GC9307N_Init;
	MPU_LCD.setcur    = GC9307N_SetCursor;
	MPU_LCD.dp 		  = GC9307N_DrawPoint;
	MPU_LCD.clear 	  = GC9307N_Clear;
	MPU_LCD.dma_clear = GC9307N_DMA_Clear;

	MPU_LCD.init();//初始化
}

void lcd_yh032qv_init(int hres, int vres)
{    
    LCD_HDOT = hres;
    LCD_VDOT = vres;
    
    MPULCD_InitStructure MPULCD_initStruct;
    MPULCD_initStruct.RDHoldTime = 10;
    MPULCD_initStruct.WRHoldTime = 3;
    MPULCD_initStruct.CSFall_WRFall = 3;
    MPULCD_initStruct.WRRise_CSRise = 3;
    MPULCD_initStruct.RDCSRise_Fall = 2;
    MPULCD_initStruct.WRCSRise_Fall = 2;
    MPULCD_Init(LCD, &MPULCD_initStruct);
    
    /* MPU屏初始化 */	
    mpu_lcd_init();
    MPU_LCD.clear(0xffff);
}

