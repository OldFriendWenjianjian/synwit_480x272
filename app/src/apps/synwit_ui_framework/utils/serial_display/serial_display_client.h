#ifndef SYNWIT_SERIAL_DISPLAY_CLIENT_DECLARE_H
#define SYNWIT_SERIAL_DISPLAY_CLIENT_DECLARE_H

#include "serial_display_opcodes.h"
#include <stdint.h>

#ifdef WIN32
    #if (defined FRAMEWORK_BUILD)
        #define SD_EXPORT_API __declspec(dllexport)
        #define SD_EXPORT_VAR __declspec(dllexport)
    #else
        #define SD_EXPORT_API __declspec(dllimport)
        #define SD_EXPORT_VAR __declspec(dllimport)
    #endif
#else
    #define SD_EXPORT_API
    #define SD_EXPORT_VAR
#endif

typedef uint16_t    sdcode_t;

/* 协议帧字段位置定义 */
#define SD_FRAME_HEADER_POS                 0   /* 2 bytes */
#define SD_FRAME_LEN_POS                    2   /* 2 bytes */
#define SD_FRAME_MSG_ID_POS                 4   /* 1 byte */
#define SD_FRAME_CMD_NUM_POS                5   /* 1 byte */
#define SD_FRAME_INSTRU_START_POS           6

/* EZOC全局模块ID定义 */
#define SD_GMODULE_ID_UI    (0x8000 + 1)
#define SD_GMODULE_ID_APP   (0x8000 + 2)

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 开始构建串口屏Setter/Getter操作数据帧
* @param[in]  buffer             数据帧缓存区
* @param[in]  size               数据帧缓存区大小
* @param[in]  msgid              用户自定义msg id，由应答数据携带返回
*/
SD_EXPORT_API void synwit_sdcmd_setter_begin(uint8_t* buffer, uint32_t size, uint8_t msgid);
SD_EXPORT_API void synwit_sdcmd_getter_begin(uint8_t* buffer, uint32_t size, uint8_t msgid);



/**@brief 往数据帧添加操作码和操作数值(推荐使用后面定义的sdcmd_xxx封装形式，而不是直接调用这个接口)
* @param[in]  op_code            操作码
* @param[in]  object_id          操作对象的ID
* @param[in]  value              构建setter帧时填入要设置的数值，getter帧时传0
*/
SD_EXPORT_API void _synwit_sdcmd_add(sdcode_t op_code, uint32_t object_id, uint32_t value);



/**@brief 数据帧构建结束
* @param[out] len                数据帧长度
* @return  调用synwit_sdcmd_begin()时传入的帧地址
*/
SD_EXPORT_API uint8_t* synwit_sdcmd_end(uint32_t *len);

#ifdef __cplusplus
} /*extern "C"*/
#endif

/* 串口数据帧构建接口，对应于EZOC属性 */
#define sdcmd_x(obj_id, value)                 _synwit_sdcmd_add(sdcode_x, obj_id, (uint32_t)(value))
#define sdcmd_y(obj_id, value)                 _synwit_sdcmd_add(sdcode_y, obj_id, (uint32_t)(value))
#define sdcmd_width(obj_id, value)             _synwit_sdcmd_add(sdcode_width, obj_id, (uint32_t)(value))
#define sdcmd_height(obj_id, value)            _synwit_sdcmd_add(sdcode_height, obj_id, (uint32_t)(value))
#define sdcmd_hidden(obj_id, value)            _synwit_sdcmd_add(sdcode_hidden, obj_id, (uint32_t)(value))
#define sdcmd_bg_color(obj_id, value)          _synwit_sdcmd_add(sdcode_bg_color, obj_id, (uint32_t)(value))
#define sdcmd_bg_grad_color(obj_id, value)     _synwit_sdcmd_add(sdcode_bg_grad_color, obj_id, (uint32_t)(value))
#define sdcmd_bg_grad_dir(obj_id, value)       _synwit_sdcmd_add(sdcode_bg_grad_dir, obj_id, (uint32_t)(value))
#define sdcmd_bg_opa(obj_id, value)            _synwit_sdcmd_add(sdcode_bg_opa, obj_id, (uint32_t)(value))
#define sdcmd_radius(obj_id, value)            _synwit_sdcmd_add(sdcode_radius, obj_id, (uint32_t)(value))
#define sdcmd_img_opa(obj_id, value)           _synwit_sdcmd_add(sdcode_img_opa, obj_id, (uint32_t)(value))

#define sdcmd_text(obj_id, str)                _synwit_sdcmd_add(sdcode_text, obj_id, (uint32_t)(str))
#define sdcmd_text_color(obj_id, value)        _synwit_sdcmd_add(sdcode_text_color, obj_id, (uint32_t)(value))
#define sdcmd_long_mode(obj_id, value)         _synwit_sdcmd_add(sdcode_long_mode, obj_id, (uint32_t)(value))
#define sdcmd_append(obj_id, value)            _synwit_sdcmd_add(sdcode_append, obj_id, (uint32_t)(value))
#define sdcmd_text_value_format(obj_id, str)   _synwit_sdcmd_add(sdcode_text_value_format)
#define sdcmd_text_value(obj_id, value)        _synwit_sdcmd_add(sdcode_text_value, obj_id, (uint32_t)(value))
#define sdcmd_text_value_max(obj_id, value)    _synwit_sdcmd_add(sdcode_text_value_max, obj_id, (uint32_t)(value))
#define sdcmd_text_value_min(obj_id, value)    _synwit_sdcmd_add(sdcode_text_value_min, obj_id, (uint32_t)(value))
#define sdcmd_text_value_loop(obj_id, value)   _synwit_sdcmd_add(sdcode_text_value_loop, obj_id, (uint32_t)(value))
#define sdcmd_suffix(obj_id, str)              _synwit_sdcmd_add(sdcode_suffix, obj_id, (uint32_t)(str))
#define sdcmd_prefix(obj_id, str)              _synwit_sdcmd_add(sdcode_prefix, obj_id, (uint32_t)(str))

#define sdcmd_click(obj_id, value)             _synwit_sdcmd_add(sdcode_click, obj_id, (uint32_t)(value))
#define sdcmd_checked(obj_id, value)           _synwit_sdcmd_add(sdcode_checked, obj_id, (uint32_t)(value))

#define sdcmd_open(obj_id, value)              _synwit_sdcmd_add(sdcode_open, obj_id, (uint32_t)(value))
#define sdcmd_sel(obj_id, value)               _synwit_sdcmd_add(sdcode_sel, obj_id, (uint32_t)(value))
#define sdcmd_add(obj_id, str)                 _synwit_sdcmd_add(sdcode_add, obj_id, (uint32_t)(str))
#define sdcmd_clear(obj_id, value)             _synwit_sdcmd_add(sdcode_clear, obj_id, (uint32_t)(value))
#define sdcmd_selected_text(obj_id, str)       _synwit_sdcmd_add(sdcode_selected_text, obj_id, (uint32_t)(str))

#define sdcmd_img_file(obj_id, str)            _synwit_sdcmd_add(sdcode_img_file, obj_id, (uint32_t)(str))
#define sdcmd_data(obj_id, value)              _synwit_sdcmd_add(sdcode_data, obj_id, (uint32_t)(value))
#define sdcmd_rotate(obj_id, value)            _synwit_sdcmd_add(sdcode_rotate, obj_id, (uint32_t)(value))
#define sdcmd_zoom(obj_id, value)              _synwit_sdcmd_add(sdcode_zoom, obj_id, (uint32_t)(value))
#define sdcmd_auto_size(obj_id, value)         _synwit_sdcmd_add(sdcode_auto_size, obj_id, (uint32_t)(value))

#define sdcmd_min(obj_id, value)               _synwit_sdcmd_add(sdcode_min, obj_id, (uint32_t)(value))
#define sdcmd_max(obj_id, value)               _synwit_sdcmd_add(sdcode_max, obj_id, (uint32_t)(value))
#define sdcmd_value(obj_id, value)             _synwit_sdcmd_add(sdcode_value, obj_id, (uint32_t)(value))
#define sdcmd_anim_time(obj_id, value)         _synwit_sdcmd_add(sdcode_anim_time, obj_id, (uint32_t)(value))

#define sdcmd_start_angle(obj_id, value)       _synwit_sdcmd_add(sdcode_start_angle, obj_id, (uint32_t)(value))
#define sdcmd_end_angle(obj_id, value)         _synwit_sdcmd_add(sdcode_end_angle, obj_id, (uint32_t)(value))
#define sdcmd_anim(obj_id, value)              _synwit_sdcmd_add(sdcode_anim, obj_id, (uint32_t)(value))

#define sdcmd_level(obj_id, value)             _synwit_sdcmd_add(sdcode_level, obj_id, (uint32_t)(value))
#define sdcmd_num(obj_id, value)               _synwit_sdcmd_add(sdcode_num, obj_id, (uint32_t)(value))
#define sdcmd_play(obj_id, value)              _synwit_sdcmd_add(sdcode_play, obj_id, (uint32_t)(value))
#define sdcmd_stop(obj_id, value)              _synwit_sdcmd_add(sdcode_stop, obj_id, (uint32_t)(value))
#define sdcmd_interval(obj_id, value)          _synwit_sdcmd_add(sdcode_interval, obj_id, (uint32_t)(value))
#define sdcmd_order(obj_id, value)             _synwit_sdcmd_add(sdcode_order, obj_id, (uint32_t)(value))
#define sdcmd_repeat(obj_id, value)            _synwit_sdcmd_add(sdcode_repeat, obj_id, (uint32_t)(value))
#define sdcmd_level_max(obj_id, value)         _synwit_sdcmd_add(sdcode_level_max, obj_id, (uint32_t)(value))
#define sdcmd_level_min(obj_id, value)         _synwit_sdcmd_add(sdcode_level_min, obj_id, (uint32_t)(value))
#define sdcmd_anim_start_level(obj_id, value)  _synwit_sdcmd_add(sdcode_anim_start_level, obj_id, (uint32_t)(value))
#define sdcmd_anim_end_level(obj_id, value)    _synwit_sdcmd_add(sdcode_anim_end_level, obj_id, (uint32_t)(value))
#define sdcmd_level_loop(obj_id, value)        _synwit_sdcmd_add(sdcode_level_loop, obj_id, (uint32_t)(value))

#define sdcmd_del(obj_id, value)               _synwit_sdcmd_add(sdcode_del, obj_id, (uint32_t)(value))

#define sdcmd_time(obj_id, str)                _synwit_sdcmd_add(sdcode_time, obj_id, (uint32_t)(str))

#define sdcmd_path(obj_id, str)                _synwit_sdcmd_add(sdcode_path, obj_id, (uint32_t)(str))
#define sdcmd_replay(obj_id, value)            _synwit_sdcmd_add(sdcode_replay, obj_id, (uint32_t)(value))
#define sdcmd_pause(obj_id, value)             _synwit_sdcmd_add(sdcode_pause, obj_id, (uint32_t)(value))
#define sdcmd_state(obj_id, value)             _synwit_sdcmd_add(sdcode_state, obj_id, (uint32_t)(value))
#define sdcmd_total_frame(obj_id, value)       _synwit_sdcmd_add(sdcode_total_frame, obj_id, (uint32_t)(value))
#define sdcmd_cur_frame(obj_id, value)         _synwit_sdcmd_add(sdcode_cur_frame, obj_id, (uint32_t)(value))
#define sdcmd_volume(obj_id, value)            _synwit_sdcmd_add(sdcode_volume, obj_id, (uint32_t)(value))
#define sdcmd_mute(obj_id, value)              _synwit_sdcmd_add(sdcode_mute, obj_id, (uint32_t)(value))
#define sdcmd_forward(obj_id, value)           _synwit_sdcmd_add(sdcode_forward, obj_id, (uint32_t)(value))
    
#define sdcmd_brush_color(obj_id, value)       _synwit_sdcmd_add(sdcode_brush_color, obj_id, (uint32_t)(value))
#define sdcmd_brush_width(obj_id, value)       _synwit_sdcmd_add(sdcode_brush_width, obj_id, (uint32_t)(value))

#define sdcmd_period(obj_id, value)            _synwit_sdcmd_add(sdcode_period, obj_id, (uint32_t)(value))
#define sdcmd_start(obj_id, value)             _synwit_sdcmd_add(sdcode_start, obj_id, (uint32_t)(value))
#define sdcmd_tick(obj_id, value)              _synwit_sdcmd_add(sdcode_tick, obj_id, (uint32_t)(value))

#define sdcmd_screen_load(str)                 _synwit_sdcmd_add(sdcode_screen_load, SD_GMODULE_ID_UI, (uint32_t)(str))
#define sdcmd_back(value)                      _synwit_sdcmd_add(sdcode_back, SD_GMODULE_ID_UI, (uint32_t)(value))
#define sdcmd_cur_screen()                     _synwit_sdcmd_add(sdcode_cur_screen, SD_GMODULE_ID_UI, (uint32_t)0)
#define sdcmd_widgets_summary()                _synwit_sdcmd_add(sdcode_widgets_summary, SD_GMODULE_ID_UI, (uint32_t)0)

#define sdcmd_msg(str)                         _synwit_sdcmd_add(sdcode_msg, SD_GMODULE_ID_APP, (uint32_t)(str))


#endif /* SYNWIT_SERIAL_DISPLAY_CLIENT_DECLARE_H */