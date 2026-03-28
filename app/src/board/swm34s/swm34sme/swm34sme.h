#ifndef BOARD_SWM34SME_H
#define BOARD_SWM34SME_H

#include "swm34s/common/uart/dev_uart.h"
#include "swm34s/common/jpeg/dev_jpeg.h"
#include "swm34s/common/lcdc/dev_lcdc.h"
#include "swm34s/common/sdram/dev_sdram.h"
#include "swm34s/common/systick/dev_systick.h"
#include "swm34s/common/dac/dev_dac.h"
#include "swm34s/common/i2s/dev_i2s.h"
#include "swm34s/common/spi/dev_lcd_spi.h"

/* 启动时，D2脚为高电平则进入U盘模式 */
#define USBMSC_CTRL_PORT     GPIOD
#define USBMSC_CTRL_PIN      PIN2

#endif /* BOARD_SWM34SME_H */