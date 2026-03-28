#ifndef SYNWIT_UI_WAV_H
#define SYNWIT_UI_WAV_H

#include "../synwit_types.h"

enum {
    WAV_VOLUME_ADJ_MODE_ABSOLUTE = 0,	// 设置音量时使用绝对值（这个模式下有效音量取值为0~100)
    WAV_VOLUME_ADJ_MODE_RELATIVE,		// 设置音量时使用相对当前音量的变化值（如-1、-5、5、10等）
};
typedef uint8_t wav_volume_adjust_mode_t;

#ifdef __cplusplus
extern "C" {
#endif

/* wav对象类型 */
typedef void    wav_obj_t;

/**@brief WAV播放完结的回调函数原型
* @param[in]  wav           wav_obj_t对象指针
* @param[in]  user_data     用户自定义数据
*/
typedef void (*on_wav_completed_cb_t)(wav_obj_t* wav_obj, void* user_data);



/**@brief 打开wav
* @param[in]  wav_path      wav声音文件的路径
* @return 打开成功返回wav_obj_t对象指针，否则返回NULL
*/
EXPORT_API wav_obj_t* synwit_wav_open(const char* wav_path);


/**@brief 关闭wav
* @param[in]  wav_obj      wav_obj_t对象指针
*/
EXPORT_API void synwit_wav_close(wav_obj_t *wav_obj);


/**@brief 阻塞方式播放wav，调用后待音频播放完毕函数才会返回
* @param[in]  wav_obj      wav_obj_t对象指针
*/
EXPORT_API void synwit_wav_play_sync(wav_obj_t* wav_obj);


/**@brief 非阻塞方式播放wav，调用后函数立即返回
* @param[in]  wav_obj      wav_obj_t对象指针
* @param[in]  prio         播放任务的优先级，为确保播放质量，推荐设置为LV_TASK_PRIO_HIGH或LV_TASK_PRIO_HIGHEST
*/
EXPORT_API void synwit_wav_play(wav_obj_t* wav_obj, lv_task_prio_t prio);


/**@brief 停止播放wav(仅对处于非阻塞方式播放下的wav有效)
* @param[in]  wav_obj      wav_obj_t对象指针
*/
EXPORT_API void synwit_wav_stop(wav_obj_t* wav_obj);



/**@brief 暂停播放wav(仅对处于非阻塞方式播放下的wav有效)
* @param[in]  wav_obj      wav_obj_t对象指针
*/
EXPORT_API void synwit_wav_pause(wav_obj_t* wav_obj);



/**@brief 设置播放结束回调函数
* @param[in]  wav_obj            wav_obj_t对象指针
* @param[in]  completed_cb       回调函数地址
* @param[in]  user_data          自定义数据，回调函数触发时传入
*/
EXPORT_API void synwit_wav_set_on_completed_cb(wav_obj_t* wav_obj,
                        on_wav_completed_cb_t completed_cb, 
                        void* user_data);



/**@brief 调整音频音量
* @param[in]  wav_obj            wav_obj_t对象指针
* @param[in]  volume   音量大小
*					   调整模式为WAV_VOLUME_ADJ_MODE_ABSOLUTE时，有效取值为0~100
*					   调整模式为WAV_VOLUME_ADJ_MODE_RELATIVE时，可以传入正负数或0
* @param[in]  adj_mode 调整模式(参考@wav_volume_adjust_mode_t)
*/
EXPORT_API void synwit_wav_set_volume(wav_obj_t* wav_obj, int volume, wav_volume_adjust_mode_t adj_mode);



/**@brief 获取wav总时长(以秒为单位)
* @param[in]  wav_obj            wav_obj_t对象指针
* @return 返回wav总时长
*/
EXPORT_API uint32_t synwit_wav_get_duration(wav_obj_t* wav_obj);



/**@brief 获取wav已播放时长(以秒为单位)
* @param[in]  wav_obj            wav_obj_t对象指针
* @return 返回已播放时长
*/
EXPORT_API uint32_t synwit_wav_get_elapsed(wav_obj_t* wav_obj);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SYNWIT_UI_WAV_H*/