#ifndef SYNWIT_UI_INTERNAL_H
#define SYNWIT_UI_INTERNAL_H

#include "lvgl/lvgl.h"
#include "synwit_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 注册界面
* @param[in]  screen_id    界面ID，定义在screen_id.h内
* @param[in]  cb           类型为@ref synwit_screen_callback_t 的界面对象，用于接收界面运行过程中的通知
* @return 注册成功返回0，失败返回<0
*/
EXPORT_API int synwit_ui_reg_screen(synwit_id_t screen_id,
                                    const synwit_screen_callback_t* cb);


#ifdef WIN32_LVGL7_SIMULATION
    void app_register_screens();
    #define SIMULATION_EXPORT __declspec(dllexport)
#else
    #ifndef WIN32
        void app_register_screens();
    #endif
    #define SIMULATION_EXPORT 
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#define REG_SCREEN(id, scr_obj) \
    extern synwit_screen_callback_t scr_obj; \
    synwit_ui_reg_screen(id, &scr_obj);

typedef struct {
    void (*rx_handler)();
    void (*notify)(const uint8_t *data, uint32_t len);
}synwit_sdisp_ops_t;

#endif /* SYNWIT_UI_INTERNAL_H */