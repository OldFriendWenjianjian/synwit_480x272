#ifndef SYNWIT_LIST_DECLARE_H
#define SYNWIT_LIST_DECLARE_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/**@brief 将上位机设定的list项的样式应用到btn上
* @param[in]  list          列表控件指针
* @param[in]  btn           一般指向自行在代码中添加到list内的btn项
*/
EXPORT_API void lv_list_decorate_btn(lv_obj_t* list, lv_obj_t *btn);


#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* SYNWIT_LIST_DECLARE_H */