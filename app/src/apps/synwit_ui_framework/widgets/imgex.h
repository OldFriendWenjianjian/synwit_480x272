#ifndef SYNWIT_IMGEX_DECLARE_H
#define SYNWIT_IMGEX_DECLARE_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 创建imgex控件
* @param[in]  par      父控件
* @param[in]  copy     一般传NULL。当copy!=NULL时，则基于这个copy控件创建一个副本
* @return 返回新建的imgex对象
*/
EXPORT_API lv_obj_t* lv_imgex_create(lv_obj_t* par, const lv_obj_t* copy);



/**@brief 设置imgex控件图片
* @param[in]  levelimg      控件指针
* @param[in]  src           图片数据源，目前仅支持使用synwit_ui_load_image_file()加载的图片源
*/
EXPORT_API void lv_imgex_set_src(lv_obj_t* obj, const void *src);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* SYNWIT_IMGEX_DECLARE_H */