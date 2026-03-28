#include "SWM341.h"
#include "lvgl/lvgl.h"
#include "lcd/ic_init/LCD_TL040WVS3.c"

void lcd_480x480_square_init(int hres, int vres)
{
    LCD_InitStructure LCD_initStruct;
    if(hres == 480 && vres == 480) {
        LCD_SPI_Init_TL040WVS3();   // SPI初始化屏
        /** 480*480 */
        LCD_initStruct.ClkDiv = 6;
        LCD_initStruct.HnPixel = 480;
        LCD_initStruct.VnPixel = 480;
        LCD_initStruct.Hfp = 10;
        LCD_initStruct.Hbp = 30;
        LCD_initStruct.Vfp = 20;
        LCD_initStruct.Vbp = 20;
        LCD_initStruct.HsyncWidth = 5;
        LCD_initStruct.VsyncWidth = 5;
        LCD_initStruct.SampleEdge = LCD_SAMPLE_RISE;
    } else {
        printf("Incompatible driver for LCD resolution.\n");
        return;
    }

#if  LV_COLOR_DEPTH == 32
    LCD_initStruct.Format = LCD_FMT_RGB888;
#elif  LV_COLOR_DEPTH == 16
    LCD_initStruct.Format = LCD_FMT_RGB565;
#endif
    LCD_initStruct.DataSource = (uint32_t)SDRAMM_BASE;
    LCD_initStruct.Background = 0xFFFFFF; //背景色
    LCD_initStruct.IntEOTEn = 0;
    LCD_Init(LCD, &LCD_initStruct);
}
