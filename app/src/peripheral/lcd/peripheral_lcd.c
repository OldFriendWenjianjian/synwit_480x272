#include "SWM341.h"
#include "lvgl/lvgl.h"
#include "synwit_ui_framework/synwit_ui.h"
#include "board.h"

#include "swm34s/common/mpu/dev_mpu.h"
volatile mpu_lcd_fun_t MPU_LCD;//ÊµÀý
volatile uint8_t flag_rgb_initialized = 0;

extern void lcd_generic_init(int hres, int vres);
extern void lcd_480x480_round_init(int hres, int vres);
extern void lcd_480x480_square_init(int hres, int vres);
extern void lcd_jlt35002a_v2_rgb_init(int hres, int vres);
extern void lcd_jlt25002a_v3_init(int hres, int vres);
extern void lcd_jlt25002a_v3_spi_init(int hres, int vres);
extern void lcd_yh032qv_init(int hres, int vres);

void peripheral_lcd_init(int hres, int vres)
{
    const char *module_name = synwit_ui_manifest_get_string("peripheral.display.driver", "generic");
    
    if(strcmp(module_name, "generic") == 0) {
        lcd_generic_init(hres, vres);
        
    } else if(strcmp(module_name, "480x480_round") == 0) {
        lcd_480x480_round_init(hres, vres);
        
    } else if(strcmp(module_name, "480x480_square") == 0) {
        lcd_480x480_square_init(hres, vres);
		
    } else if(strcmp(module_name, "jlt35002a_v2_rgb") == 0) {
        lcd_jlt35002a_v2_rgb_init(hres, vres);
      
    } else if(strcmp(module_name, "jlt35002a_v3") == 0) {
        lcd_jlt25002a_v3_init(hres, vres);
    } else if(strcmp(module_name, "jlt35002a_v3_spi") == 0) {
        lcd_jlt25002a_v3_spi_init(hres, vres);
    } else if(strcmp(module_name, "yh032qv") == 0) {
        lcd_yh032qv_init(hres, vres);
    } else {
        printf("Unknown display driver module '%s'\n", module_name);
    }
}