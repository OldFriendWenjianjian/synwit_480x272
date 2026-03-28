#include "SWM341.h"
#include "lvgl/lvgl.h"
#include "lcd/ic_init/LCD_JLT35002A_V2.c"

void lcd_jlt35002a_v2_rgb_init(int hres, int vres)
{
    LCD_InitStructure LCD_initStruct;
    if ( (hres == 480 && vres == 320) || (hres == 320 && vres == 480) ) {
		lcd_spi_init_JLT35002A(hres, vres);   // SPI³õÊ¼»¯ÆÁ
		/** 480*320 || 320*480 */
        LCD_initStruct.HnPixel = hres;
        LCD_initStruct.VnPixel = vres;
		LCD_initStruct.ClkDiv = 10;
		LCD_initStruct.Hfp = 4;
		LCD_initStruct.Hbp = 4;
		LCD_initStruct.Vfp = 2;
		LCD_initStruct.Vbp = 1;
		LCD_initStruct.HsyncWidth = 2;
		LCD_initStruct.VsyncWidth = 5;
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
    LCD_initStruct.Background = 0xFFFFFF; //±³¾°É«
    LCD_initStruct.IntEOTEn = 0;
    LCD_Init(LCD, &LCD_initStruct);
}
