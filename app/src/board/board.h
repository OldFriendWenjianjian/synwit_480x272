#ifndef APP_BOARD_H
#define APP_BOARD_H
#include <stdio.h>

#if defined(SWM34SVET6_A1) || defined(SWM34SVET6_A3)
    #include "swm34s/swm34svet6/swm34svet6.h"
#elif defined(SWM34SRE_A1)
    #include "swm34s/swm34sre/swm34sre.h"
#elif defined(SWM34SCE_A1)
    #include "swm34s/swm34sce/swm34sce.h"
#elif defined(SWM34SME_A1)
    #include "swm34s/swm34sme/swm34sme.h"
#endif

void board_tp_init(void);
void lcdc_start(void);

// LCD控制器驱动模式：
//    LCDC_DRIVE_MODE_RGB: RGB模式
//    LCDC_DRIVE_MODE_MPU: MPU模式
//    LCDC_DRIVE_MODE_SPI: SPI模式
//    LCDC_DRIVE_MODE_SPI_DMA: SPI DMA模式
#define LCDC_DRIVE_MODE_RGB     0
#define LCDC_DRIVE_MODE_MPU     1
#define LCDC_DRIVE_MODE_SPI     2
#define LCDC_DRIVE_MODE_SPI_DMA 3

void lcdc_set_drive_mode(uint8_t mode);
uint8_t lcdc_get_drive_mode();

#endif /* APP_BOARD_H */