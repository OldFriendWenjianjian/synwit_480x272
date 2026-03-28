#ifndef LV7_DATA_PROVIDER_H
#define LV7_DATA_PROVIDER_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif


int data_provider_fs_read(uint32_t offset, void *buf, uint32_t size, uint32_t always_zero);
int data_provider_spiflash_read(uint32_t offset, void *buf, uint32_t size, uint32_t always_zero);


#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* LV7_DATA_PROVIDER_H */