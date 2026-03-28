#ifndef SYNWIT_UI_PNG_H
#define SYNWIT_UI_PNG_H

#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief PNG图像转imgex格式
* @param[in]  png_file          png文件路径
* @param[in]  width             返回png图片宽度
* @param[in]  height            返回png图片高度
* @return  转换成功返回0，否则返回<0
*/
EXPORT_API int synwit_png_get_info(const char* png_file,
                                int* width, int* height);



/**@brief PNG图像转imgex格式
* @param[in]  png_file          png文件路径
* @param[in]  resize_width      缩放宽度，传0表示保持原图宽度
* @param[in]  resize_height     缩放高度，传0表示保持原图高度
* @param[in]  dithering_level   消除水波纹，0表示无需额外处理，4表示进行最强烈的消除水波纹处理
* @param[in]  quality           画质级别(0~3，分别对应低、中、高、极高画质)
* @param[in]  out_file          输出文件路径
* @return  转换成功返回0，否则返回<0
*/
EXPORT_API int synwit_png_to_imgex(const char *png_file, 
    int resize_width, int resize_height, int dithering_level, int quality,
    const char *out_file);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SYNWIT_UI_FONT_H*/