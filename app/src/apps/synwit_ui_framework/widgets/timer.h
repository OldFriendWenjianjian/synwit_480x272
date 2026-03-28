#ifndef SYNWIT_TIMER_DECLARE_H
#define SYNWIT_TIMER_DECLARE_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/**@brief 启动定时器
* @param[in]  timer     timer控件指针
*/
EXPORT_API void lv_timer_start(lv_obj_t* timer);



/**@brief 停止定时器
* @param[in]  timer     timer控件指针
*/
EXPORT_API void lv_timer_stop(lv_obj_t* timer);


/**@brief 设置定时器触发间隔
* @param[in]  timer     timer控件指针
* @param[in]  period    触发间隔时间，以毫秒为单位
*/
EXPORT_API void lv_timer_set_period(lv_obj_t* timer, uint32_t period);



/**@brief 设置定时器触发次数
* @param[in]  timer         timer控件指针
* @param[in]  repeat_count  触发次数，0表示无限次数
*/
EXPORT_API void lv_timer_set_repeat(lv_obj_t* timer, uint32_t repeat_count);



/**@brief 获取定时器触发间隔
* @param[in]  timer         timer控件指针
* @return 返回触发间隔
*/
EXPORT_API uint32_t lv_timer_get_period(lv_obj_t* timer);



/**@brief 获取定时器最大重复次数
* @param[in]  timer         timer控件指针
* @return 返回最大重复次数
*/
EXPORT_API uint32_t lv_timer_get_repeat(lv_obj_t* timer);



/**@brief 设置一个虚拟tick数值，这个虚拟tick数值会在每次定时器触发时简单+1
* @param[in]  timer         timer控件指针
* @param[in]  tick          tick数值
*/
EXPORT_API void lv_timer_set_tick(lv_obj_t* timer, uint32_t tick);



/**@brief 获取当前虚拟tick数值
* @param[in]  timer         timer控件指针
* @return 返回当前tick的数值
*/
EXPORT_API uint32_t lv_timer_get_tick(lv_obj_t* timer);


#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* SYNWIT_TIMER_DECLARE_H */