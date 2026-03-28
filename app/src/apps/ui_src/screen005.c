#include "synwit_ui_framework/synwit_ui.h"
#include "appkit/screen_id.h"
#include "appkit/screens/screen005.h"

/*
 * 界面回调函数
 */
static void screen005_on_timer(void *user_data)
{
    /* 界面定时器回调函数 */
}

static void screen005_start(synwit_id_t screen_id)
{
    /* 在界面被显示给用户前，这个接口会被调用。 */

    // 打开下面的注释可以为本界面开启一个每100ms触发一次的定时器
    //synwit_ui_start_scr_timer(100, NULL);
}

static void screen005_stop(synwit_id_t screen_id)
{
    /* 准备切换到其它界面前，这个接口会被调用。 */
}

static int screen005_favorite_stop_policy()
{
    /* 函数返回值表示本界面结束时，希望采用的资源管理策略 */

    /* 资源管理策略(主要影响本界面再次进入时的加载速度):
     *    0: 系统推荐，释放当前界面引用的资源，保留控件及控件状态。可获得平衡的界面切换性能和内存消耗
     *    1: 释放当前界面引用的资源，并删除所有控件。
     *    2: 保留当前界面的所有资源及控件。可以获得最佳的界面切换性能，但内存消耗较大
     */
    return 0;
}


/*
 * 界面注册对象
 */
synwit_screen_callback_t screen005_cb_obj = {
    .favorite_stop_policy = screen005_favorite_stop_policy,
    .on_screen_start = screen005_start,
    .on_screen_stop = screen005_stop,
    .on_screen_timer = screen005_on_timer,
};

