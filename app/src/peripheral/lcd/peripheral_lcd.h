#ifndef PERIPHERAL_LCD_H
#define PERIPHERAL_LCD_H

#include "SWM341.h"
#include "swm34s/common/mpu/dev_mpu.h"

extern volatile uint8_t flag_rgb_initialized;
extern volatile mpu_lcd_fun_t MPU_LCD;//ÊµÀý

void peripheral_lcd_init(int hres, int vres);

#endif //PERIPHERAL_LCD_H