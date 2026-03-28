#ifndef SYNWIT_ARTCHAR_DECLARE_H
#define SYNWIT_ARTCHAR_DECLARE_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 设置艺术字控件的文字内容
* @param[in]  artchar    艺术字控件指针
* @param[in]  text       文字内容
* @return 设置成功返回0，否则返回<0
*/
EXPORT_API int lv_artchar_set_text(lv_obj_t* artchar, const char* text);


/**@brief 获取艺术字控件的文字内容
* @param[in]  artchar    艺术字控件指针
* @return 返回文字缓存区的指针
*/
EXPORT_API char * lv_artchar_get_text(const lv_obj_t* artchar);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* SYNWIT_ARTCHAR_DECLARE_H */