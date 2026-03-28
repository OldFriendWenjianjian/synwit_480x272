#ifndef SYNWIT_UI_H
#define SYNWIT_UI_H

#include "lvgl/lvgl.h"
#include "synwit_types.h"
#include "synwit_widgets.h"
#include "synwit_utils.h"
#include "synwit_ezoc.h"
#include "synwit_serial_display.h"

#include "misc/preference.h"
#include "misc/font.h"
#include "misc/wav.h"
#include "misc/png.h"

typedef enum {
    MLH_RES_NORMAL = 0,                 // 照常执行lv_task_handler
    MLH_RES_IGNORE_LV_TASK_HANDLER,     // 不继续执行lv_task_handler
}mainloop_hook_res_t;

typedef struct {
    void (*framework_ready)(void);
    lv_obj_t* (*build_entry_screen)(void);
    void (*app_ready)(void);

    // 读取ui.bin偏移offset开始，大小为size字节的数据至buf缓存
    // 返回实际读取的字节数，出错返回<0
    int (*ui_data_read)(uint32_t offset, void* buf, uint32_t size, uint32_t always_zero);

    void (*main_tick)(void);
    /* 指定main_tick回调函数的调度间隔，毫秒为单位，默认为50ms */
    uint32_t main_tick_period; 
    /* 指定main_tick回调函数的优先级，默认为LV_TASK_PRIO_HIGH */
    uint8_t main_tick_prio;

    /* 钩子函数，每次主循环执行lv_task_handler前，回调这个函数 */
    mainloop_hook_res_t (*lv_task_handler_pre_hook)(void);

    /* 钩子函数，进入蓝屏界面前会被调用 */
    void (*pre_blue)(const char* msg);

    /* 全局钩子函数，进入新界面后会被调用 */
    void (*on_screen_start)(synwit_id_t screen_id);
    /* 全局钩子函数，离开当前界面前会被调用 */
    void (*on_screen_stop)(synwit_id_t screen_id);

    /* 响应EZOC消息，当执行EZOC命令app.msg = "xxxx"时会触发这个回调 */
    void (*on_ezoc_msg)(const char *msg, uint32_t len);
}synwit_app_callback_t;

typedef struct {
    void (*on_screen_start)(synwit_id_t screen_id);
    void (*on_screen_stop)(synwit_id_t screen_id);
    void (*on_screen_timer)(void *data);

    /* 函数返回值表示所属界面结束时，希望采用的资源管理策略:
     *      0: 系统推荐，释放当前界面引用的资源，保留控件及控件状态。可获得平衡的界面切换性能和内存消耗
     *      1: 释放当前界面引用的资源，并删除所有控件
     *      2: 保留当前界面的所有资源及控件。可以获得最佳的界面切换性能，但内存消耗较大
     */
    int (*favorite_stop_policy)();
}synwit_screen_callback_t;

typedef enum {
    SCR_TRANSITION_DIR_NONE = 0,

    SCR_TRANSITION_DIR_LEFT,                /* 向左 */
    SCR_TRANSITION_DIR_RIGHT,               /* 向右 */
    SCR_TRANSITION_DIR_UP,                  /* 向上 */
    SCR_TRANSITION_DIR_DOWN,                /* 向下 */
}synwit_scr_transition_dir_t;

typedef enum {
    SCR_TRANSITION_EFFECT_NONE = 0,

    SCR_TRANSITION_EFFECT_SHIFT,            /* 平移 */
    SCR_TRANSITION_EFFECT_COVER,            /* 遮盖 */
    SCR_TRANSITION_EFFECT_LIFT,             /* 掀起 */
    SCR_TRANSITION_EFFECT_FADE,             /* 渐隐渐现 */
}synwit_scr_transition_effect_t;

typedef struct {
    /* 开关控制 */
    uint32_t disable_fast_preload : 1;      /* 设为1则在加载界面时，屏蔽高速预载功能 */
    uint32_t del_current_screen : 1;        /* 跳转前删除当前界面(控件状态不保留) */

    /* 控制界面切换时的过渡效果 */
    synwit_scr_transition_dir_t transition_dir;         /* 过渡方向 */
    synwit_scr_transition_effect_t transition_effect;   /* 过渡特效 */
    uint32_t transition_time_ms;                        /* 过渡总时间 */
}synwit_load_scr_dsc_t;

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 启动UI框架(阻塞)
* @param[in]  data_root    指定UI数据所在的路径
* @param[in]  app_cb       类型为@ref synwit_app_callback_t 的回调对象，用于接收UI框架启动过程中的通知
*/
EXPORT_API void synwit_ui_start(const char* data_root, const synwit_app_callback_t* app_cb);



/**@brief 在当前界面内查找指定控件
* @param[in]  widget_id    控件ID
* @return 返回对应控件的lv_obj_t指针，失败返回NULL
*/
EXPORT_API lv_obj_t* synwit_ui_find_lv_obj(synwit_id_t widget_id);
EXPORT_API lv_obj_t* synwit_ui_find_lv_obj_by_name(const char* name);



/**@brief 通过名称获取界面ID
* @param[in]  name      界面名称
* @return 返回对应界面的ID，找不到则返回0
*/
EXPORT_API synwit_id_t synwit_ui_get_screen_id_by_name(const char* name);



/**@brief 跳转到指定界面
* @param[in]  screen    可以传入以下2类界面：
*                        - 上位机构建界面的ID，定义于文件screen_id.h中
*                        - LVGL的screen对象
* @return 跳转成功返回0，失败返回<0
*/
EXPORT_API int synwit_ui_load_screen(synwit_id_t screen);



/**@brief 初始化synwit_load_scr_dsc_t结构体
* @param[in]  dsc    要初始化的结构体指针
*/
EXPORT_API void synwit_ui_init_load_scr_dsc(synwit_load_scr_dsc_t* dsc);



/**@brief 按描述结构的内容，以指定方式跳转到界面
* @param[in]  screen    可以传入以下2类界面：
*                        - 上位机构建界面的ID，定义于文件screen_id.h中
*                        - LVGL的screen对象
* @param[in]  dsc       描述结构指针，参考@synwit_load_scr_dsc_t
* @return 跳转成功返回0，失败返回<0
*/
EXPORT_API int synwit_ui_load_screen_with_dsc(synwit_id_t screen, const synwit_load_scr_dsc_t *dsc);



/**@brief 过渡到指定界面
* @param[in]  screen        上位机构建界面的ID，定义于文件screen_id.h中
* @param[in]  dir           过渡方向，参考@synwit_scr_transition_dir_t
* @param[in]  effect        过渡效果，参考@synwit_scr_transition_effect_t
* @param[in]  anim_time_ms  过渡时间(毫秒)
* @return 跳转成功返回0，失败返回<0
*/
EXPORT_API int synwit_ui_transition_screen(synwit_id_t screen_id,
    synwit_scr_transition_dir_t dir,
    synwit_scr_transition_effect_t effect,
    uint32_t anim_time_ms);


/**@brief 为当前界面启动定时器
* @param[in]  period        timer回调的触发间隔(ms)
* @param[in]  user_data     timer回调时传入的用户自定义数据
* @return 成功返回0，失败返回<0
*/
EXPORT_API int synwit_ui_start_scr_timer(uint32_t period, void *user_data);



/**@brief 停止当前界面的定时器
* @param[in]  void
* @return 成功返回0，失败返回<0
*/
EXPORT_API int synwit_ui_stop_scr_timer();



/**@brief 读取SynwitManifest.cfg内的配置信息(字符串类型)
* @param[in]  name      配置名称，例如LCD驱动模块"peripheral.display.driver"
* @param[in]  def       配置不存在时返回的默认字符串
* @return  配置名对应的字符串
*/
EXPORT_API const char* synwit_ui_manifest_get_string(const char* name, const char* def);



/**@brief 读取SynwitManifest.cfg内的配置信息(整数类型)
* @param[in]  name      配置名称
* @param[in]  def       配置不存在时返回的默认数值
* @return  配置名对应的数值
*/
EXPORT_API int synwit_ui_manifest_get_int(const char* name, int def);



/**@brief 从磁盘加载图像，支持IMGEX bin格式图像、JPEG图像、LVGL bin格式图像(自动识别),LVGL bin文件支持以下色彩模式:
*               CF_TRUE_COLOR
*               CF_TRUE_COLOR_ALPHA
*               CF_TRUE_COLOR_CHROMA
* @param[in]  path      文件路径
* @return  LVGL的图像描述符
*/
EXPORT_API const lv_img_dsc_t* synwit_ui_load_image_file(const char* path);



/**@brief 释放之前从磁盘加载的图像
* @param[in]  img_dsc      synwit_ui_load_image_lvbin()或synwit_ui_load_image_jpeg()返回的指针
*/
EXPORT_API void synwit_ui_unload_image(const lv_img_dsc_t* img_dsc);



/**@brief 返回当前界面的screen id
* @param[in]  void
* @return  当前界面的screen id，出错则返回0
*/
EXPORT_API synwit_id_t synwit_ui_get_cur_screen_id();



/**@brief 获取平台版本名
* @param[in]  ver_name      保存版本名的缓存
* @param[in]  size          缓存大小
*/
EXPORT_API void synwit_ui_get_platform_version_name(char *ver_name, uint32_t size);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* SYNWIT_UI_H */