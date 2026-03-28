#ifndef SYNWIT_UI_FONT_H
#define SYNWIT_UI_FONT_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 获取字库
* @param[in]  font_name     字库名称
* @return  返回字库对象指针，失败则返回NULL
*/
EXPORT_API const lv_font_t* synwit_ui_font_get(const char *font_name);



/**@brief 加载外部字库(仅支持hmf格式)
* @param[in]  font_path         字库文件
* @param[in]  load_in_memory    true则整个字库加载到RAM中(性能好，消耗内存大)
*                               false则实时从磁盘读取字型(速度慢，但节省内存)
* @return  返回字库对象指针，失败则返回NULL
*/
EXPORT_API lv_font_t* synwit_ui_font_load(const char* font_path, bool load_in_memory);



/**@brief 释放外部字库
* @param[in]  font         synwit_ui_font_load()返回的字库对象
*/
EXPORT_API void synwit_ui_font_unload(lv_font_t* font);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SYNWIT_UI_FONT_H*/