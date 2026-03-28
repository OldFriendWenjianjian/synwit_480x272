#ifndef SYNWIT_SERIAL_DISPLAY_OPCODE_DECLARE_H
#define SYNWIT_SERIAL_DISPLAY_OPCODE_DECLARE_H

#define PTYPE_NUMBER       0
#define PTYPE_STRING       1

#define PTYPE_I16          PTYPE_NUMBER
#define PTYPE_U8           PTYPE_NUMBER
#define PTYPE_U16          PTYPE_NUMBER
#define PTYPE_U32          PTYPE_NUMBER
#define PTYPE_DOUBLE       PTYPE_NUMBER

#define SDCODE_MAKE_DEF(op_code, ptype) \
    (sdcode_t)(0UL | (op_code) | ((ptype) << 12))

#define sdcode_x                  SDCODE_MAKE_DEF(1, PTYPE_I16)
#define sdcode_y                  SDCODE_MAKE_DEF(2, PTYPE_I16)
#define sdcode_width              SDCODE_MAKE_DEF(3, PTYPE_I16)
#define sdcode_height             SDCODE_MAKE_DEF(4, PTYPE_I16)
#define sdcode_hidden             SDCODE_MAKE_DEF(5, PTYPE_U8)
#define sdcode_bg_color           SDCODE_MAKE_DEF(6, PTYPE_U32)
#define sdcode_bg_grad_color      SDCODE_MAKE_DEF(7, PTYPE_U32)
#define sdcode_bg_grad_dir        SDCODE_MAKE_DEF(8, PTYPE_U8)
#define sdcode_bg_opa             SDCODE_MAKE_DEF(9, PTYPE_U8)
#define sdcode_radius             SDCODE_MAKE_DEF(10, PTYPE_I16)
#define sdcode_img_opa            SDCODE_MAKE_DEF(11, PTYPE_U8)

#define sdcode_text               SDCODE_MAKE_DEF(12, PTYPE_STRING)
#define sdcode_text_color         SDCODE_MAKE_DEF(13, PTYPE_U32)
#define sdcode_long_mode          SDCODE_MAKE_DEF(14, PTYPE_U8)
#define sdcode_append             SDCODE_MAKE_DEF(15, PTYPE_STRING)
#define sdcode_text_value_format  SDCODE_MAKE_DEF(16, PTYPE_STRING)
#define sdcode_text_value         SDCODE_MAKE_DEF(17, PTYPE_DOUBLE)
#define sdcode_text_value_max     SDCODE_MAKE_DEF(18, PTYPE_DOUBLE)
#define sdcode_text_value_min     SDCODE_MAKE_DEF(19, PTYPE_DOUBLE)
#define sdcode_text_value_loop    SDCODE_MAKE_DEF(20, PTYPE_U8)
#define sdcode_suffix             SDCODE_MAKE_DEF(21, PTYPE_STRING)
#define sdcode_prefix             SDCODE_MAKE_DEF(22, PTYPE_STRING)

#define sdcode_click              SDCODE_MAKE_DEF(23, PTYPE_U8)

#define sdcode_checked            SDCODE_MAKE_DEF(24, PTYPE_U8)

#define sdcode_open               SDCODE_MAKE_DEF(25, PTYPE_U8)
#define sdcode_sel                SDCODE_MAKE_DEF(26, PTYPE_U16)
#define sdcode_add                SDCODE_MAKE_DEF(27, PTYPE_STRING)
#define sdcode_clear              SDCODE_MAKE_DEF(28, PTYPE_U8)
#define sdcode_selected_text      SDCODE_MAKE_DEF(29, PTYPE_STRING)

#define sdcode_img_file           SDCODE_MAKE_DEF(30, PTYPE_STRING)
#define sdcode_data               SDCODE_MAKE_DEF(31, PTYPE_U32)
#define sdcode_rotate             SDCODE_MAKE_DEF(32, PTYPE_I16)
#define sdcode_zoom               SDCODE_MAKE_DEF(33, PTYPE_I16)
#define sdcode_auto_size          SDCODE_MAKE_DEF(34, PTYPE_U8)

#define sdcode_min                SDCODE_MAKE_DEF(35, PTYPE_I16)
#define sdcode_max                SDCODE_MAKE_DEF(36, PTYPE_I16)
#define sdcode_value              SDCODE_MAKE_DEF(37, PTYPE_I16)
#define sdcode_anim_time          SDCODE_MAKE_DEF(38, PTYPE_U16)

#define sdcode_start_angle        SDCODE_MAKE_DEF(39, PTYPE_U16)
#define sdcode_end_angle          SDCODE_MAKE_DEF(40, PTYPE_U16)
#define sdcode_anim               SDCODE_MAKE_DEF(41, PTYPE_U8)

#define sdcode_level              SDCODE_MAKE_DEF(42, PTYPE_U16)
#define sdcode_num                SDCODE_MAKE_DEF(43, PTYPE_U16)
#define sdcode_play               SDCODE_MAKE_DEF(44, PTYPE_U8)
#define sdcode_stop               SDCODE_MAKE_DEF(45, PTYPE_U8)
#define sdcode_interval           SDCODE_MAKE_DEF(46, PTYPE_U32)
#define sdcode_order              SDCODE_MAKE_DEF(47, PTYPE_U8)
#define sdcode_repeat             SDCODE_MAKE_DEF(48, PTYPE_U16)
#define sdcode_level_max          SDCODE_MAKE_DEF(49, PTYPE_U16)
#define sdcode_level_min          SDCODE_MAKE_DEF(50, PTYPE_U16)
#define sdcode_anim_start_level   SDCODE_MAKE_DEF(51, PTYPE_U16)
#define sdcode_anim_end_level     SDCODE_MAKE_DEF(52, PTYPE_U16)
#define sdcode_level_loop         SDCODE_MAKE_DEF(53, PTYPE_U8)

#define sdcode_del                SDCODE_MAKE_DEF(54, PTYPE_U8)

#define sdcode_time               SDCODE_MAKE_DEF(55, PTYPE_STRING)

#define sdcode_path               SDCODE_MAKE_DEF(56, PTYPE_STRING)
#define sdcode_replay             SDCODE_MAKE_DEF(57, PTYPE_U8)
#define sdcode_pause              SDCODE_MAKE_DEF(58, PTYPE_U8)
#define sdcode_state              SDCODE_MAKE_DEF(59, PTYPE_U16)
#define sdcode_total_frame        SDCODE_MAKE_DEF(60, PTYPE_U32)
#define sdcode_cur_frame          SDCODE_MAKE_DEF(61, PTYPE_U32)
#define sdcode_volume             SDCODE_MAKE_DEF(62, PTYPE_I16)
#define sdcode_mute               SDCODE_MAKE_DEF(63, PTYPE_U8)
#define sdcode_forward            SDCODE_MAKE_DEF(64, PTYPE_U8)

#define sdcode_brush_color        SDCODE_MAKE_DEF(65, PTYPE_U32)
#define sdcode_brush_width        SDCODE_MAKE_DEF(66, PTYPE_I16)

#define sdcode_period             SDCODE_MAKE_DEF(67, PTYPE_U32)
#define sdcode_start              SDCODE_MAKE_DEF(68, PTYPE_U8)
#define sdcode_tick               SDCODE_MAKE_DEF(69, PTYPE_U32)

#define sdcode_screen_load        SDCODE_MAKE_DEF(70, PTYPE_STRING)
#define sdcode_back               SDCODE_MAKE_DEF(71, PTYPE_U8)
#define sdcode_cur_screen         SDCODE_MAKE_DEF(72, PTYPE_STRING)
#define sdcode_widgets_summary    SDCODE_MAKE_DEF(73, PTYPE_STRING)

#define sdcode_msg                SDCODE_MAKE_DEF(74, PTYPE_STRING)

#endif /* SYNWIT_SERIAL_DISPLAY_CLIENT_DECLARE_H */