#include "SWM341.h"
#include "board.h"

static uint8_t s_lcdc_drive_mode = LCDC_DRIVE_MODE_RGB;

void lcdc_start(void)
{
    // MPU模式下不能调用LCD_Start()，否则会导致屏幕部分区域出现花屏
    if(lcdc_get_drive_mode() == LCDC_DRIVE_MODE_RGB) {
        LCD_Start(LCD);
    }
}

void lcdc_set_drive_mode(uint8_t mode)
{
    s_lcdc_drive_mode = mode;
}

uint8_t lcdc_get_drive_mode()
{
    return s_lcdc_drive_mode;
}