#include "main.h"
#include "MassStorage.h"
#include "FlashDisk.h"
#include "app_usbmsc.h"

void app_usbmsc_handler(void)
{
    GPIO_Init(USBMSC_CTRL_PORT, USBMSC_CTRL_PIN, 0, 0, 1, 0); //????,?????U???
    for (uint32_t i = 0; i < CyclesPerUs * 1000; i++)
        __NOP();
    
    if(GPIO_GetBit(USBMSC_CTRL_PORT, USBMSC_CTRL_PIN) == 1)
    {
        uint32_t n=0;
        MSC_Init();
        USBD_Open();
        while(1==1)
        {
            MSC_ProcessOUT();
            
            if(++n % 1000000 == 0)
            {
                FlashDiskFlush();
            }
        }
    }
}