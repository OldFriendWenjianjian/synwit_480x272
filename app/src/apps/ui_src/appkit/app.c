#include "synwit_ui_framework/synwit_ui.h"
#include "synwit_ui_framework/synwit_ui_internal.h"
#include "screen_id.h"

/*
 * Disable serial display
 */
synwit_sdisp_ops_t g_sdisp_ops = {
    .rx_handler = NULL,
    .notify = NULL,
};

#define REG_WIDGET_CLASS(cls) \
    extern void cls##_register(); \
    cls##_register();

#define REG_ACTION(cls) \
    extern void action_##cls##_register(); \
    action_##cls##_register();

#define EZOC_IMPORT(cls_name, ezoc_obj) \
    EXPORT_API int synwit_ezoc_add_ops(const char* cname, const void* ops); \
    EXPORT_VAR extern const void* ezoc_obj; \
    synwit_ezoc_add_ops(#cls_name, ezoc_obj);

#define USE_EZOC_INTERPRETER() \
    EXPORT_VAR extern void* ezoc_interpreter; \
    EXPORT_API void synwit_ui_use_interpreter(void* interpreter); \
    synwit_ui_use_interpreter(ezoc_interpreter);


void app_register_widget_classes(void)
{
    REG_WIDGET_CLASS(cls_bar);
    REG_WIDGET_CLASS(cls_button);
    REG_WIDGET_CLASS(cls_image);
    REG_WIDGET_CLASS(cls_label);
    REG_WIDGET_CLASS(cls_screen);
    REG_WIDGET_CLASS(cls_widget);

    /*
     * Classes below are unused.
     */
    //REG_WIDGET_CLASS(cls_checkbox);
    //REG_WIDGET_CLASS(cls_slider);
    //REG_WIDGET_CLASS(cls_switch);
    //REG_WIDGET_CLASS(cls_image_btn);
    //REG_WIDGET_CLASS(cls_list);
    //REG_WIDGET_CLASS(cls_artchar);
    //REG_WIDGET_CLASS(cls_roller);
    //REG_WIDGET_CLASS(cls_dropdown);
    //REG_WIDGET_CLASS(cls_arc);
    //REG_WIDGET_CLASS(cls_touch);
    //REG_WIDGET_CLASS(cls_levelimg);
    //REG_WIDGET_CLASS(cls_chart);
    //REG_WIDGET_CLASS(cls_textarea);
    //REG_WIDGET_CLASS(cls_imgroller);
    //REG_WIDGET_CLASS(cls_clock);
    //REG_WIDGET_CLASS(cls_avi);
    //REG_WIDGET_CLASS(cls_canvas);
    //REG_WIDGET_CLASS(cls_timer);
}

void app_register_actions(void)
{

    /*
     * Actions below are unused.
     */
    //REG_ACTION(goto_screen);
    //REG_ACTION(set_visibility);
    //REG_ACTION(set_text_color);
    //REG_ACTION(set_bg_color);
    //REG_ACTION(set_text);
    //REG_ACTION(set_font_by_name);
    //REG_ACTION(back);
    //REG_ACTION(set_text_with_color_and_font);
    //REG_ACTION(slide_to_screen);
    //REG_ACTION(play_avi);
    //REG_ACTION(pause_avi);
    //REG_ACTION(stop_avi);
    //REG_ACTION(replay_avi);
    //REG_ACTION(execute);
}

void app_register_ezocs(void)
{
    static int user_ezoc_registered = 0;
    if (!user_ezoc_registered) {
        /*
         * UI EZOC components
         */
        EZOC_IMPORT(bar, _framework_bar_ezoc_obj);
        EZOC_IMPORT(button, _framework_button_ezoc_obj);
        EZOC_IMPORT(image, _framework_image_ezoc_obj);
        EZOC_IMPORT(label, _framework_label_ezoc_obj);
        EZOC_IMPORT(screen, _framework_screen_ezoc_obj);
        EZOC_IMPORT(widget, _framework_widget_ezoc_obj);

        /*
         * Global EZOC components
         */
        EZOC_IMPORT(ui, _framework_ui_ezoc_obj);
        EZOC_IMPORT(app, _framework_app_ezoc_obj);
        EZOC_IMPORT(var, _framework_var_ezoc_obj);

        /*
         * EZOCs below are unused.
         */
        //EZOC_IMPORT(checkbox, _framework_checkbox_ezoc_obj);
        //EZOC_IMPORT(slider, _framework_slider_ezoc_obj);
        //EZOC_IMPORT(switch, _framework_switch_ezoc_obj);
        //EZOC_IMPORT(image_btn, _framework_image_btn_ezoc_obj);
        //EZOC_IMPORT(list, _framework_list_ezoc_obj);
        //EZOC_IMPORT(artchar, _framework_artchar_ezoc_obj);
        //EZOC_IMPORT(roller, _framework_roller_ezoc_obj);
        //EZOC_IMPORT(dropdown, _framework_dropdown_ezoc_obj);
        //EZOC_IMPORT(arc, _framework_arc_ezoc_obj);
        //EZOC_IMPORT(touch, _framework_touch_ezoc_obj);
        //EZOC_IMPORT(levelimg, _framework_levelimg_ezoc_obj);
        //EZOC_IMPORT(chart, _framework_chart_ezoc_obj);
        //EZOC_IMPORT(textarea, _framework_textarea_ezoc_obj);
        //EZOC_IMPORT(imgroller, _framework_imgroller_ezoc_obj);
        //EZOC_IMPORT(clock, _framework_clock_ezoc_obj);
        //EZOC_IMPORT(avi, _framework_avi_ezoc_obj);
        //EZOC_IMPORT(canvas, _framework_canvas_ezoc_obj);
        //EZOC_IMPORT(timer, _framework_timer_ezoc_obj);

        USE_EZOC_INTERPRETER();
        user_ezoc_registered = 1;
    }
}

void app_register_screens(void)
{
    /*
     * 注册界面
     */
    REG_SCREEN(SCREEN001, screen001_cb_obj);
    REG_SCREEN(SCREEN002, screen002_cb_obj);
    REG_SCREEN(SCREEN003, screen003_cb_obj);
    REG_SCREEN(SCREEN004, screen004_cb_obj);
    REG_SCREEN(SCREEN005, screen005_cb_obj);
    REG_SCREEN(SCREEN006, screen006_cb_obj);
    REG_SCREEN(SCREEN007, screen007_cb_obj);
    REG_SCREEN(SCREEN008, screen008_cb_obj);
    REG_SCREEN(SCREEN009, screen009_cb_obj);

    app_register_widget_classes();
    app_register_actions();
}

