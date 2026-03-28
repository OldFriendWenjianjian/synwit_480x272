#ifndef SYNWIT_TEXTAREA_DECLARE_H
#define SYNWIT_TEXTAREA_DECLARE_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 编辑框软键盘弹出时的回调函数
* @param[in]  ta            编辑框控件指针
* @param[in]  kb            软键盘控件指针(lv_keyboard类型)
* @param[in]  user_data     用户自定义数据
*/
typedef void (*lv_textarea_on_kb_create_cb_t)(lv_obj_t* ta, lv_obj_t* kb, void* user_data);


/**@brief 设置软键盘创建完成后的回调，可用于在用户端重设软键盘样式等
* @param[in]  ta            编辑框控件指针
* @param[in]  kb_create_cb  回调函数
* @param[in]  user_data     自定义数据，回调函数触发时传入
*/
EXPORT_API void lv_textarea_set_on_kb_create_cb(lv_obj_t* ta,
                            lv_textarea_on_kb_create_cb_t kb_create_cb, 
                            void* user_data);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* SYNWIT_TEXTAREA_DECLARE_H */