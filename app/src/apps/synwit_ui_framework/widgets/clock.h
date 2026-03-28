#ifndef SYNWIT_CLOCK_DECLARE_H
#define SYNWIT_CLOCK_DECLARE_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 通过字符串设置时钟控件的时间
* @param[in]  clock      控件指针
* @param[in]  time_str   时间字符串，例如"15:36:40"、"14:07"等
* @return 设置成功返回0，否则返回<0
*/
EXPORT_API int lv_clock_set_time_string(lv_obj_t* clock, const char *time_str);


/**@brief 通过数值设置时钟控件的时间
* @param[in]  clock      控件指针
* @param[in]  hours      小时数，有效范围0~24
* @param[in]  minutes    分钟数，有效范围0~60
* @param[in]  seconds    秒数，有效范围0~60
* @return 设置成功返回0，否则返回<0
*/
EXPORT_API int lv_clock_set_time(lv_obj_t* clock, uint16_t hours, uint16_t minutes, uint16_t seconds);


/**@brief 停止自动动画演示
* @param[in]  clock      控件指针
*/
EXPORT_API void lv_clock_stop_auto_anim(lv_obj_t* clock);


/**@brief 开始自动动画演示
* @param[in]  clock      控件指针
* @param[in]  period     模拟1s的时间间隔，以毫秒为单位
*/
EXPORT_API void lv_clock_start_auto_anim(lv_obj_t* clock, uint32_t period);
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* SYNWIT_CLOCK_DECLARE_H */