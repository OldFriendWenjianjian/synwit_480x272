#ifndef SYNWIT_AVI_DECLARE_H
#define SYNWIT_AVI_DECLARE_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	AVI_STATE_STOPPED = 0,
	AVI_STATE_PLAYING,
	AVI_STATE_PAUSE
};
typedef uint8_t avi_state_t;

enum {
	AVI_STOP_STYLE_NONE = 0,		// 播放停止后保持当前画面(默认)
	AVI_STOP_STYLE_DEFAULT_COVER,	// 播放停止后显示上位机设置的封面图片
	AVI_STOP_STYLE_FIRST_FRAME		// 播放停止后显示视频首帧画面
};
typedef uint8_t avi_stop_style_t;

enum {
	AVI_VOLUME_ADJ_MODE_ABSOLUTE = 0,	// 设置音量时使用绝对值（这个模式下有效音量取值为0~100)
	AVI_VOLUME_ADJ_MODE_RELATIVE,		// 设置音量时使用相对当前音量的变化值（如-1、-5、5、10等）
};
typedef uint8_t avi_volume_adjust_mode_t;

/**@brief AVI/WAV播放完结的回调函数原型
* @param[in]  avi           avi控件指针
* @param[in]  user_data     用户自定义数据
*/
typedef void (*lv_avi_on_completed_cb_t)(lv_obj_t* avi, void *user_data);


/**@brief 设置播放源
* @param[in]  avi           avi控件指针
* @param[in]  src_path      媒体文件路径，支持AVI/WAV，以及播放列表文件
* @return 设置成功返回0，否则返回<0
*/
EXPORT_API int lv_avi_set_src(lv_obj_t* avi, const char *src_path);



/**@brief 设置播放结束回调函数
* @param[in]  avi           avi控件指针
* @param[in]  stop_cb       回调函数地址
* @param[in]  user_data     自定义数据，回调函数触发时传入
*/
EXPORT_API void lv_avi_set_on_completed_cb(lv_obj_t* avi, 
                        lv_avi_on_completed_cb_t completed_cb, void *user_data);



/**@brief 开始播放
* @param[in]  avi      avi控件指针
*/
EXPORT_API void lv_avi_play(lv_obj_t* avi);



/**@brief 停止播放
* @param[in]  avi      avi控件指针
*/
EXPORT_API void lv_avi_stop(lv_obj_t* avi);



/**@brief 暂停播放
* @param[in]  avi      avi控件指针
*/
EXPORT_API void lv_avi_pause(lv_obj_t* avi);



/**@brief 重新从头开始播放
* @param[in]  avi      avi控件指针
*/
EXPORT_API void lv_avi_replay(lv_obj_t* avi);



/**@brief 设置重播次数
* @param[in]  avi      avi控件指针
* @param[in]  repeat   重播次数，设置为0表示无限重播
*/
EXPORT_API void lv_avi_set_repeat_count(lv_obj_t* avi, uint32_t repeat);



/**@brief 设置播放停止时的样式
* @param[in]  avi      avi控件指针
* @param[in]  style    停止播放后AVI控件的样式(参考@avi_stop_style_t)
*/
EXPORT_API void lv_avi_set_stop_style(lv_obj_t* avi, avi_stop_style_t style);



/**@brief 获取当前播放状态
* @param[in]  avi      avi控件指针
* @return 返回当前播放状态
*/
EXPORT_API avi_state_t lv_avi_get_state(lv_obj_t* avi);



/**@brief 获取AVI视频总帧数
* @param[in]  avi      avi控件指针
* @return 返回视频总帧数
*/
EXPORT_API uint32_t lv_avi_get_total_frame(lv_obj_t* avi);



/**@brief 获取AVI视频当前帧序号
* @param[in]  avi      avi控件指针
* @return 返回当前播放帧序号
*/
EXPORT_API uint32_t lv_avi_get_cur_frame_index(lv_obj_t* avi);



/**@brief 获取总时长
* @param[in]  avi      avi控件指针
* @return 返回音频总时长，以秒为单位
*/
EXPORT_API uint32_t lv_avi_get_duration(lv_obj_t* avi);



/**@brief 获取已播放时长
* @param[in]  avi      avi控件指针
* @return 返回已播放时长，以秒为单位
*/
EXPORT_API uint32_t lv_avi_get_elapsed(lv_obj_t* avi);



/**@brief 关联用户自定样式的进度条(进度条必须为lv_bar类型的控件)
* @param[in]  avi      avi控件指针
* @param[in]  bar      lv_bar类型的控件指针
* @return 返回当前播放帧序号
*/
EXPORT_API void lv_avi_set_progress_bar(lv_obj_t* avi, lv_obj_t *bar);



/**@brief 开启/关闭音频流输出
* @param[in]  avi      avi控件指针
* @param[in]  enabled  true为打开音频流输出，false为关闭音频流输出
*/
EXPORT_API void lv_avi_enable_audio_output(lv_obj_t* avi, bool enabled);



/**@brief 调整音频音量
* @param[in]  avi      avi控件指针
* @param[in]  volume   音量大小
*					   调整模式为AVI_VOLUME_ADJ_MODE_ABSOLUTE时，有效取值为0~100
*					   调整模式为AVI_VOLUME_ADJ_MODE_RELATIVE时，可以传入正负数或0
* @param[in]  adj_mode 调整模式(参考@avi_volume_adjust_mode_t)
*/
EXPORT_API void lv_avi_set_volume(lv_obj_t* avi, int volume, avi_volume_adjust_mode_t adj_mode);



/**@brief 获取当前音频音量
* @param[in]  avi      avi控件指针
* @return 返回当前音量
*/
EXPORT_API uint16_t lv_avi_get_volume(lv_obj_t* avi);



/**@brief 切换到下一个媒体文件(仅在使用播放列表文件时有效)
* @param[in]  avi      avi控件指针
* @return 保留，当前返回值一律为0
*/
EXPORT_API int lv_avi_forward(lv_obj_t* avi);



/**@brief 创建avi控件
* @param[in]  par		父控件
* @param[in]  reserved	固定传NULL。
* @return 返回新建的imgex对象
*/
EXPORT_API lv_obj_t* lv_avi_create(lv_obj_t* par, const lv_obj_t* reserved);



/**@brief 删除由lv_avi_create()创建的avi控件
* @param[in]  avi		avi控件
*/
EXPORT_API void lv_avi_del(lv_obj_t* avi);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /* SYNWIT_AVI_DECLARE_H */