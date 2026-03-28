#ifndef SYNWIT_CANVAS_DECLARE_H
#define SYNWIT_CANVAS_DECLARE_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/**@brief 设置画笔颜色
* @param[in]  canvas    canvas控件指针
* @param[in]  color     颜色
*/
EXPORT_API void lv_canvas_set_brush_color(lv_obj_t* canvas, lv_color_t color);



/**@brief 设置画笔粗细
* @param[in]  canvas    canvas控件指针
* @param[in]  width     画笔粗细
*/
EXPORT_API void lv_canvas_set_brush_width(lv_obj_t* canvas, int width);



/**@brief 使用画笔绘制线条
* @param[in]  canvas        canvas控件指针
* @param[in]  points        点信息列表
* @param[in]  point_cnt     点个数
*/
EXPORT_API void lv_canvas_draw_line_by_brush(lv_obj_t* canvas, const lv_point_t *points, uint32_t point_cnt);



/**@brief 使用背景色擦除画布
* @param[in]  canvas        canvas控件指针
*/
EXPORT_API void lv_canvas_clear(lv_obj_t* canvas);


#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* SYNWIT_CANVAS_DECLARE_H */