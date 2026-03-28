#ifndef SYNWIT_UI_GPS_TRACKER_H
#define SYNWIT_UI_GPS_TRACKER_H
#include "lvgl/lvgl.h"
#include "../../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 创建GPS轨迹对象
* @param[in]  canvas				一个画布对象，GPS轨迹将绘制在这个画布上
* @param[in]  max_trajectories		最大能记录的轨迹点，超过则循环覆盖最旧的轨迹点
* @return 返回GPS轨迹对象指针，出错则返回NULL
*/
EXPORT_API void* gps_tracker_create(lv_obj_t *canvas, uint32_t max_trajectories);



/**@brief 删除GPS轨迹对象
* @param[in]  tracker				由gps_tracker_create()创建的对象
*/
EXPORT_API void gps_tracker_del(void* tracker);



/**@brief 设置坐标原点在画布里的位置(默认位于画布中心位置)
* @param[in]  tracker	轨迹对象
* @param[in]  x,ｙ		相对于画布左上角的坐标
*/
EXPORT_API void gps_tracker_set_origin(void* tracker, int x, int y);



/**@brief 修改画布指代的区域大小(以米为单位，默认为500米)
* @param[in]  tracker	轨迹对象
* @param[in]  meters	画布指代的实际物理区域大小，如果画布宽高不相等，则以长边为准
*/
EXPORT_API void gps_tracker_set_view_range(void* tracker, int32_t meters);



/**@brief 获取画布指代的物理区域大小
* @param[in]  tracker	轨迹对象
* @return 返回画布当前指代的实际物理区域大小(以米为单位)
*/
EXPORT_API int32_t gps_tracker_get_view_range(void* tracker);



/**@brief 设置轨迹线条的粗细(默认为8个像素)
* @param[in]  tracker		轨迹对象
* @param[in]  line_width	以像素为单位的线宽
*/
EXPORT_API void gps_tracker_set_line_width(void* tracker, uint16_t line_width);



/**@brief 设置轨迹线条的着色(默认为红色)
* @param[in]  tracker		轨迹对象
* @param[in]  color	        颜色值
*/
EXPORT_API void gps_tracker_colorize_line(void* tracker, lv_color_t color);



/**@brief 向轨迹对象添加一个经纬度坐标点
* @param[in]  tracker		轨迹对象
* @param[in]  lon			经度
* @param[in]  lat			纬度
*/
EXPORT_API void gps_tracker_feed(void* tracker, double lon, double lat);



/**@brief 在当前位置做一个图案标记
* @param[in]  tracker		轨迹对象
* @param[in]  img_dsc		标记图案的描述结构，参考@ref lv_img_dsc_t
*/
EXPORT_API void gps_tracker_tag(void* tracker, const lv_img_dsc_t* img_dsc);



/**@brief 清除所有的标记图案
* @param[in]  tracker		轨迹对象
*/
EXPORT_API void gps_tracker_clear_tags(void* tracker);



/**@brief 设置旋转朝向时的平滑过渡步长(最小为l°，默认为3°)
* @param[in]  tracker		轨迹对象
* @param[in]  angle_step	角度步长，设置为0则旋转朝向时没有过渡效果
*/
EXPORT_API void gps_tracker_set_spin_transition(void* tracker, uint8_t angle_step);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* SYNWIT_UI_GPS_TRACKER_H */