#ifndef SYNWIT_LEVELIMG_DECLARE_H
#define SYNWIT_LEVELIMG_DECLARE_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 动画播放完结的回调函数原型
* @param[in]  levelimg      控件指针
* @param[in]  user_data     用户自定义数据
*/
typedef void (*lv_levelimg_anim_ready_cb_t)(lv_obj_t* levelimg, void *user_data);


/**@brief 设置分级图片的级别
* @param[in]  levelimg      控件指针
* @param[in]  level         级数，设0则显示第0张图，设1则显示第1张图，以此类推
* @return 设置成功返回0，否则返回<0
*/
EXPORT_API int lv_levelimg_set_level(lv_obj_t* levelimg, uint16_t level);


/**@brief 设置动画演示总时间【调用后播放模式会切换为总时长优先模式(会跳帧)】
* @param[in]  levelimg      控件指针
* @param[in]  positive_ms   正序播放总时长，以毫秒(ms)为单位，设置为0则不进行正序播放
* @param[in]  reverse_ms    逆序播放总时长，以毫秒(ms)为单位，设置为0则不进行逆序播放
* @param[in]  repeat_count  循环播放次数，0表示进行无限次循环播放
*/
EXPORT_API void lv_levelimg_set_playtime(lv_obj_t* levelimg, 
    uint32_t positive_ms,
    uint32_t reverse_ms,
    uint16_t repeat_count);


/**@brief 设置动画帧间隔【调用后播放模式会切换为完整播放模式(不跳帧)】
* @param[in]  levelimg          控件指针
* @param[in]  interval_ms       每帧间隔时间，以毫秒(ms)为单位
* @param[in]  positive_order    true表示需要进行正序播放,false则不进行正序播放
* @param[in]  reverse_order     true表示需要进行逆序播放,false则不进行逆序播放
* @param[in]  repeat_count      循环播放次数，0表示进行无限次循环播放
*/
EXPORT_API void lv_levelimg_set_frame_interval(lv_obj_t* levelimg,
    uint32_t interval_ms,
    bool positive_order,
    bool reverse_order,
    uint16_t repeat_count);


/**@brief 设置动画结束回调函数(仅当有限次数循环播放时有效）
* @param[in]  levelimg      控件指针
* @param[in]  ready_cb      回调函数地址
* @param[in]  user_data     自定义数据，回调函数触发时传入
*/
EXPORT_API void lv_levelimg_set_ready_cb(lv_obj_t* levelimg, 
    lv_levelimg_anim_ready_cb_t ready_cb, void *user_data);


/**@brief 获取分级图片的总数
* @param[in]  levelimg      控件指针
* @return 该控件包含的图片总数
*/
EXPORT_API int lv_levelimg_get_level_num(lv_obj_t* levelimg);


/**@brief 停止自动动画演示
* @param[in]  levelimg      控件指针
*/
EXPORT_API void lv_levelimg_stop_auto_anim(lv_obj_t* levelimg);


/**@brief 开始自动动画演示
* @param[in]  levelimg      控件指针
*/
EXPORT_API void lv_levelimg_start_auto_anim(lv_obj_t* levelimg);


/**@brief 设置自动动画演示的起始级和结束级
* @param[in]  levelimg      控件指针
* @param[in]  start_level   起始级，传入-1表示保持现有起始级不变
* @param[in]  end_level     结束级，传入-1表示保持现有结束级不变
*/
EXPORT_API void lv_levelimg_set_anim_range(lv_obj_t* levelimg, int16_t start_level, int16_t end_level);


/**@brief 获取分级图片的当前级别
* @param[in]  levelimg      控件指针
* @return 返回当前级别
*/
EXPORT_API uint16_t lv_levelimg_get_level(lv_obj_t* levelimg);


/**@brief 获取分级图片自动动画时的帧间隔、重复次数、是否正序/逆序播放等信息
* @param[in]  levelimg      控件指针
* @return 返回相应信息
*/
EXPORT_API uint32_t lv_levelimg_get_interval(lv_obj_t* levelimg);
EXPORT_API uint16_t lv_levelimg_get_repeat_count(lv_obj_t* levelimg);
EXPORT_API bool lv_levelimg_get_positive_order(lv_obj_t* levelimg);
EXPORT_API bool lv_levelimg_get_reverse_order(lv_obj_t* levelimg);
EXPORT_API uint16_t lv_levelimg_get_anim_start_level(lv_obj_t* levelimg);
EXPORT_API uint16_t lv_levelimg_get_anim_end_level(lv_obj_t* levelimg);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* SYNWIT_LEVELIMG_DECLARE_H */