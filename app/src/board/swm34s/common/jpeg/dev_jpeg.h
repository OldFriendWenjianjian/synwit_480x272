#ifndef __DEV_JPEG_H__
#define __DEV_JPEG_H__

#include "SWM341.h"
#include <math.h>
#include <string.h>

void jpeg_init(void);
void jpeg_decode(JPEG_TypeDef *JPEGx, jfif_info_t *jfif_info, jpeg_outset_t *jpeg_outset, uint32_t stride);

#endif //__DEV_JPEG_H__
