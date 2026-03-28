#include "SWM341.h"
#include "lvgl/lvgl.h"

void lcd_generic_init(int hres, int vres)
{
    LCD_InitStructure LCD_initStruct;
    if(hres == 800 && vres == 480) {
        /** 800*480 */
        LCD_initStruct.ClkDiv = 5;
        LCD_initStruct.HnPixel = 800;
        LCD_initStruct.VnPixel = 480;
        LCD_initStruct.Hfp = 64;
        LCD_initStruct.Hbp = 46;
        LCD_initStruct.Vfp = 22;
        LCD_initStruct.Vbp = 23;
        LCD_initStruct.HsyncWidth = 5;
        LCD_initStruct.VsyncWidth = 5;
        LCD_initStruct.SampleEdge = LCD_SAMPLE_FALL;
    } else if(hres == 480 && vres == 272) {
        /** 480*272 */
        LCD_initStruct.ClkDiv = 15;
        LCD_initStruct.HnPixel = 480;
        LCD_initStruct.VnPixel = 272;
        LCD_initStruct.Hfp = 5;
        LCD_initStruct.Hbp = 40;
        LCD_initStruct.Vfp = 8;
        LCD_initStruct.Vbp = 8;
        LCD_initStruct.HsyncWidth = 5;
        LCD_initStruct.VsyncWidth = 5;
        LCD_initStruct.SampleEdge = LCD_SAMPLE_FALL;
    } else if(hres == 1024 && vres == 600) {
        /** 1024*600 */
        LCD_initStruct.ClkDiv = 3;
        LCD_initStruct.HnPixel = 1024;
        LCD_initStruct.VnPixel = 600;
        LCD_initStruct.Hfp = 64;
        LCD_initStruct.Hbp = 140;
        LCD_initStruct.Vfp = 12;
        LCD_initStruct.Vbp = 12;
        LCD_initStruct.HsyncWidth = 20;
        LCD_initStruct.VsyncWidth = 3;
        LCD_initStruct.SampleEdge = LCD_SAMPLE_FALL;
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
