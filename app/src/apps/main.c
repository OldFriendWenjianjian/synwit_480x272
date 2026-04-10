#include "main.h"
#include "bootscreen.h"
#include "lv_port_jpg.h"
#include "FlashDisk.h"
#include "MassStorage.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "board.h"
#include "lcd/peripheral_lcd.h"
#include "synwit_ui_framework/synwit_ui.h"
#include "synwit_ui_framework/synwit_ui_internal.h"
#include "app_usbmsc.h"
#include "ff.h"
#include "../data_provider/data_provider.h"
#include "pressure/pressure_manager.h"
#include <stdlib.h>
#include <string.h>

#if defined(SWM34SVET6_A3)
#include "swm34s/swm34svet6/a3/dev_i2c1_sensor.h"
#endif

#define TAG "app"
#define MANIFEST_DIR    "S:spi:/"

extern synwit_sdisp_ops_t g_sdisp_ops;

struct soft_uart_probe_desc
{
    const char *uart_name;
    const char *tx_name;
    GPIO_TypeDef *tx_gpio;
    PORT_TypeDef *tx_port;
    uint8_t tx_pin;
    const char *rx_name;
    GPIO_TypeDef *rx_gpio;
    PORT_TypeDef *rx_port;
    uint8_t rx_pin;
    const char *runtime_note;
};

static void mount_external_fs();
static void user_init();
static void pressure_ui_init(void);
static void pressure_ui_update(bool force);
static void pressure_debug_update(bool force);
static void pressure_i2c_scan_once(void);
static void pressure_ui_refresh_detail_live(void);
static void pressure_ui_load_detail_points(void);
static void pressure_ui_open_detail(uint8_t sensor_index);
static void pressure_ui_close_detail(void);
static void pressure_ui_card_event_cb(lv_obj_t *obj, lv_event_t event);
static void pressure_ui_cal_btn_event_cb(lv_obj_t *obj, lv_event_t event);
static void pressure_ui_capture_btn_event_cb(lv_obj_t *obj, lv_event_t event);
static void pressure_ui_save_point_btn_event_cb(lv_obj_t *obj, lv_event_t event);
static void pressure_ui_clear_point_btn_event_cb(lv_obj_t *obj, lv_event_t event);
static void pressure_ui_detail_action_event_cb(lv_obj_t *obj, lv_event_t event);
static void pressure_ui_textarea_event_cb(lv_obj_t *obj, lv_event_t event);
static void pressure_ui_keyboard_event_cb(lv_obj_t *obj, lv_event_t event);
static void pressure_ui_route_text(const pressure_manager_route_info_t *route_info, char *buf, size_t size);
static bool pressure_ui_parse_mmhg_x100(const char *text, int32_t *value_x100);
static void pressure_ui_format_mmhg(char *buf, size_t size, float value, const char *unit);
static void pressure_ui_format_x100(char *buf, size_t size, int32_t value_x100);
static void soft_uart_probe_init(void);
static void soft_uart_probe_poll(void);
static void soft_uart_probe_prepare_line(const struct soft_uart_probe_desc *desc);
static void soft_uart_probe_send_frame(const struct soft_uart_probe_desc *desc, const uint8_t *data, uint32_t len);
static bool soft_uart_probe_receive_byte(const struct soft_uart_probe_desc *desc, uint8_t *data, uint32_t timeout_ms);
static uint32_t soft_uart_probe_receive_window(const struct soft_uart_probe_desc *desc, uint8_t *buffer, uint32_t max_len, uint32_t window_ms);
static void soft_uart_probe_log_rx(const uint8_t *buffer, uint32_t len);

#define pressure_display_init   pressure_ui_init
#define pressure_display_update pressure_ui_update

typedef struct {
    lv_obj_t *card;
    lv_obj_t *title_label;
    lv_obj_t *value_label;
    lv_obj_t *route_label;
    lv_obj_t *status_label;
    lv_obj_t *calibration_label;
    lv_obj_t *calibration_button;
} pressure_ui_card_t;

typedef struct {
    lv_obj_t *ref_ta;
    lv_obj_t *measured_ta;
    lv_obj_t *raw_label;
    lv_obj_t *capture_button;
    lv_obj_t *save_button;
    lv_obj_t *clear_button;
} pressure_ui_point_row_t;

static lv_obj_t *s_pressure_root = NULL;
static lv_obj_t *s_pressure_header_label = NULL;
static lv_obj_t *s_pressure_count_label = NULL;
static lv_obj_t *s_pressure_page = NULL;
static lv_obj_t *s_pressure_empty_label = NULL;
static pressure_ui_card_t s_pressure_cards[PRESSURE_MANAGER_MAX_SENSORS];
static lv_obj_t *s_pressure_detail = NULL;
static lv_obj_t *s_pressure_detail_title = NULL;
static lv_obj_t *s_pressure_detail_value = NULL;
static lv_obj_t *s_pressure_detail_status = NULL;
static pressure_ui_point_row_t s_pressure_point_rows[PRESSURE_MANAGER_MAX_CAL_POINTS];
static lv_obj_t *s_pressure_detail_close_button = NULL;
static lv_obj_t *s_pressure_detail_clear_button = NULL;
static lv_obj_t *s_pressure_keyboard = NULL;
static uint8_t s_pressure_detail_sensor = 0xFFU;
static uint32_t s_pressure_display_tick = 0U;
static uint32_t s_pressure_debug_tick = 0U;
static bool s_pressure_i2c_scan_done = false;

#define PRESSURE_UI_CARD_HEIGHT 58
#define PRESSURE_UI_CARD_GAP    6

/* Debug-only: busy-waits in main_tick() and disables IRQs while bit-banging. */
#define SOFT_UART_PROBE_ENABLE         0U
#define SOFT_UART_PROBE_PERIOD_MS      10000U
#define SOFT_UART_PROBE_RX_WINDOW_MS   10U
#define SOFT_UART_PROBE_BIT_US         104U
#define SOFT_UART_PROBE_START_GUARD_US 52U
#define SOFT_UART_PROBE_POLL_US        8U
#define SOFT_UART_PROBE_RX_MAX_LEN     8U

static const struct soft_uart_probe_desc s_soft_uart_probe_descs[] = {
    {
        .uart_name = "UART1",
        .tx_name = "PD10",
        .tx_gpio = GPIOD,
        .tx_port = PORTD,
        .tx_pin = PIN10,
        .rx_name = "PD11",
        .rx_gpio = GPIOD,
        .rx_port = PORTD,
        .rx_pin = PIN11,
        .runtime_note = "no board conflict found in current project"
    },
    {
        .uart_name = "UART2",
        .tx_name = "PD12",
        .tx_gpio = GPIOD,
        .tx_port = PORTD,
        .tx_pin = PIN12,
        .rx_name = "PD13",
        .rx_gpio = GPIOD,
        .rx_port = PORTD,
        .rx_pin = PIN13,
        .runtime_note = "no board conflict found in current project"
    },
    {
        .uart_name = "UART3",
        .tx_name = "PD14",
        .tx_gpio = GPIOD,
        .tx_port = PORTD,
        .tx_pin = PIN14,
        .rx_name = "PD15",
        .rx_gpio = GPIOD,
        .rx_port = PORTD,
        .rx_pin = PIN15,
        .runtime_note = "no board conflict found in current project"
    },
    {
        .uart_name = "UART4",
        .tx_name = "PB3",
        .tx_gpio = GPIOB,
        .tx_port = PORTB,
        .tx_pin = PIN3,
        .rx_name = "PB4",
        .rx_gpio = GPIOB,
        .rx_port = PORTB,
        .rx_pin = PIN4,
        .runtime_note = "no board conflict found in current project"
    },
    {
        .uart_name = "UART5",
        .tx_name = "PC6",
        .tx_gpio = GPIOC,
        .tx_port = PORTC,
        .tx_pin = PIN6,
        .rx_name = "PC7",
        .rx_gpio = GPIOC,
        .rx_port = PORTC,
        .rx_pin = PIN7,
        .runtime_note = "no board conflict found in current project"
    },
    {
        .uart_name = "UART6",
        .tx_name = "PM7",
        .tx_gpio = GPIOM,
        .tx_port = PORTM,
        .tx_pin = PIN7,
        .rx_name = "PM9",
        .rx_gpio = GPIOM,
        .rx_port = PORTM,
        .rx_pin = PIN9,
        .runtime_note = "no board conflict found in current project"
    },
    {
        .uart_name = "UART7",
        .tx_name = "PM10",
        .tx_gpio = GPIOM,
        .tx_port = PORTM,
        .tx_pin = PIN10,
        .rx_name = "PA12",
        .rx_gpio = GPIOA,
        .rx_port = PORTA,
        .rx_pin = PIN12,
        .runtime_note = "PA12 is available in current LV_COLOR_DEPTH=16 project"
    },
    {
        .uart_name = "UART8",
        .tx_name = "PA13",
        .tx_gpio = GPIOA,
        .tx_port = PORTA,
        .tx_pin = PIN13,
        .rx_name = "PC8",
        .rx_gpio = GPIOC,
        .rx_port = PORTC,
        .rx_pin = PIN8,
        .runtime_note = "PA13/PC8 are available in current LV_COLOR_DEPTH=16 project"
    }
};

static uint32_t s_soft_uart_probe_tick = 0U;
static uint32_t s_soft_uart_probe_index = 0U;
static bool s_soft_uart_probe_inited = false;

static void pressure_ui_format_mmhg(char *buf, size_t size, float value, const char *unit)
{
    int32_t scaled = (value >= 0.0f) ? (int32_t)(value * 100.0f + 0.5f) : (int32_t)(value * 100.0f - 0.5f);
    int32_t integer = scaled / 100;
    int32_t fraction = scaled % 100;

    if(fraction < 0) {
        fraction = -fraction;
    }

    snprintf(buf, size, "%ld.%02ld %s", (long)integer, (long)fraction, unit);
}

static void pressure_ui_format_x100(char *buf, size_t size, int32_t value_x100)
{
    int32_t integer = value_x100 / 100;
    int32_t fraction = value_x100 % 100;

    if(fraction < 0) {
        fraction = -fraction;
    }

    snprintf(buf, size, "%ld.%02ld", (long)integer, (long)fraction);
}

static bool pressure_ui_parse_mmhg_x100(const char *text, int32_t *value_x100)
{
    const char *cursor = text;
    int32_t sign = 1;
    int32_t integer = 0;
    int32_t fraction = 0;
    int32_t scale = 10;
    bool has_digit = false;

    if((text == NULL) || (value_x100 == NULL)) {
        return false;
    }

    while((*cursor == ' ') || (*cursor == '\t')) {
        cursor++;
    }

    if(*cursor == '-') {
        sign = -1;
        cursor++;
    } else if(*cursor == '+') {
        cursor++;
    }

    while((*cursor >= '0') && (*cursor <= '9')) {
        has_digit = true;
        integer = integer * 10 + (int32_t)(*cursor - '0');
        cursor++;
    }

    if(*cursor == '.') {
        cursor++;
        while((*cursor >= '0') && (*cursor <= '9') && (scale > 0)) {
            has_digit = true;
            fraction += (int32_t)(*cursor - '0') * scale;
            scale /= 10;
            cursor++;
        }
        while((*cursor >= '0') && (*cursor <= '9')) {
            cursor++;
        }
    }

    while((*cursor == ' ') || (*cursor == '\t')) {
        cursor++;
    }

    if((has_digit == false) || (*cursor != '\0')) {
        return false;
    }

    *value_x100 = sign * (integer * 100 + fraction);
    return true;
}

static void pressure_ui_route_text(const pressure_manager_route_info_t *route_info, char *buf, size_t size)
{
    if((route_info == NULL) || (buf == NULL) || (size == 0U)) {
        return;
    }

    if(route_info->route_type == PRESSURE_MANAGER_ROUTE_DIRECT) {
        snprintf(buf, size, "Direct @0x%02X", (unsigned int)route_info->sensor_addr);
    } else {
        snprintf(buf, size, "TCA 0x%02X / CH%u",
                 (unsigned int)route_info->mux_addr,
                 (unsigned int)route_info->mux_channel);
    }
}

static int32_t pressure_ui_find_sensor_index_from_obj(lv_obj_t *obj)
{
    uint32_t i;

    for(i = 0U; i < PRESSURE_MANAGER_MAX_SENSORS; i++) {
        if((s_pressure_cards[i].card == obj) || (s_pressure_cards[i].calibration_button == obj)) {
            return (int32_t)i;
        }
    }

    return -1;
}

static int32_t pressure_ui_find_point_index_from_obj(lv_obj_t *obj)
{
    uint32_t i;

    for(i = 0U; i < PRESSURE_MANAGER_MAX_CAL_POINTS; i++) {
        if((s_pressure_point_rows[i].capture_button == obj) ||
           (s_pressure_point_rows[i].save_button == obj) ||
           (s_pressure_point_rows[i].clear_button == obj)) {
            return (int32_t)i;
        }
    }

    return -1;
}

static void pressure_ui_destroy_keyboard(void)
{
    if(s_pressure_keyboard != NULL) {
        lv_obj_del(s_pressure_keyboard);
        s_pressure_keyboard = NULL;
    }
}

static void pressure_ui_keyboard_event_cb(lv_obj_t *obj, lv_event_t event)
{
    lv_keyboard_def_event_cb(obj, event);

    if((event == LV_EVENT_APPLY) || (event == LV_EVENT_CANCEL)) {
        pressure_ui_destroy_keyboard();
    }
}

static void pressure_ui_textarea_event_cb(lv_obj_t *obj, lv_event_t event)
{
    (void)obj;
    (void)event;
}

static void pressure_ui_refresh_detail_live(void)
{
    pressure_manager_data_t pressure_data;
    pressure_manager_route_info_t route_info;
    char title_text[48];
    char value_text[48];
    char status_text[96];
    char route_text[32];

    if((s_pressure_detail == NULL) || lv_obj_get_hidden(s_pressure_detail) || (s_pressure_detail_sensor >= PRESSURE_MANAGER_MAX_SENSORS)) {
        return;
    }

    if((pressure_manager_get_data(s_pressure_detail_sensor, &pressure_data) == false) ||
       (pressure_manager_get_route_info(s_pressure_detail_sensor, &route_info) == false)) {
        return;
    }

    pressure_ui_route_text(&route_info, route_text, sizeof(route_text));
    snprintf(title_text, sizeof(title_text), "Sensor %u  %s",
             (unsigned int)(s_pressure_detail_sensor + 1U),
             route_text);

    if(pressure_data.data_valid && pressure_data.online && pressure_data.zero_calibrated) {
        char measured_text[32];

        pressure_ui_format_mmhg(value_text, sizeof(value_text), pressure_data.pressure_mmhg, "mmHg");
        pressure_ui_format_mmhg(measured_text, sizeof(measured_text), pressure_data.measured_pressure_mmhg, "mmHg");
        snprintf(status_text, sizeof(status_text), "Meas %s  Raw %lu  Pts %u/5  Tap to return",
                 measured_text,
                 (unsigned long)pressure_data.filtered_counts,
                 (unsigned int)pressure_data.calibration_point_count);
    } else if(pressure_data.data_valid && pressure_data.online) {
        snprintf(value_text, sizeof(value_text), "Zeroing");
        snprintf(status_text, sizeof(status_text), "Baseline %u/%u  Tap to return",
                 (unsigned int)pressure_data.zero_sample_count,
                 (unsigned int)PRESSURE_MANAGER_ZERO_CALIBRATION_SAMPLES);
    } else if(pressure_data.busy) {
        snprintf(value_text, sizeof(value_text), "Reading");
        snprintf(status_text, sizeof(status_text), "Sensor busy  Tap to return");
    } else {
        snprintf(value_text, sizeof(value_text), "Offline");
        snprintf(status_text, sizeof(status_text), "Last err %u  Tap to return",
                 (unsigned int)pressure_data.last_error);
    }

    lv_label_set_text(s_pressure_detail_title, title_text);
    lv_label_set_text(s_pressure_detail_value, value_text);
    lv_label_set_text(s_pressure_detail_status, status_text);
}

static void pressure_ui_load_detail_points(void)
{
    pressure_manager_calibration_t calibration;
    uint32_t i;
    char text[24];
    char raw_text[32];

    if((s_pressure_detail_sensor >= PRESSURE_MANAGER_MAX_SENSORS) ||
       (pressure_manager_get_calibration(s_pressure_detail_sensor, &calibration) == false)) {
        return;
    }

    for(i = 0U; i < PRESSURE_MANAGER_MAX_CAL_POINTS; i++) {
        if(calibration.points[i].valid) {
            pressure_ui_format_x100(text, sizeof(text), calibration.points[i].reference_mmhg_x100);
            lv_textarea_set_text(s_pressure_point_rows[i].ref_ta, text);

            pressure_ui_format_x100(text, sizeof(text), calibration.points[i].measured_mmhg_x100);
            lv_textarea_set_text(s_pressure_point_rows[i].measured_ta, text);
            snprintf(raw_text, sizeof(raw_text), "Raw %lu",
                     (unsigned long)calibration.points[i].filtered_counts);
            lv_label_set_text(s_pressure_point_rows[i].raw_label, raw_text);
        } else {
            lv_textarea_set_text(s_pressure_point_rows[i].ref_ta, "");
            lv_textarea_set_text(s_pressure_point_rows[i].measured_ta, "");
            lv_label_set_text(s_pressure_point_rows[i].raw_label, "Raw --");
        }
    }

    pressure_ui_refresh_detail_live();
}

static void pressure_ui_open_detail(uint8_t sensor_index)
{
    if((s_pressure_detail == NULL) || (sensor_index >= pressure_manager_get_sensor_count())) {
        return;
    }

    pressure_ui_destroy_keyboard();
    s_pressure_detail_sensor = sensor_index;
    lv_obj_set_hidden(s_pressure_detail, false);
    pressure_ui_load_detail_points();
}

static void pressure_ui_close_detail(void)
{
    pressure_ui_destroy_keyboard();
    s_pressure_detail_sensor = 0xFFU;
    if(s_pressure_detail != NULL) {
        lv_obj_set_hidden(s_pressure_detail, true);
    }
}

static void pressure_ui_card_event_cb(lv_obj_t *obj, lv_event_t event)
{
    int32_t sensor_index;

    if(event != LV_EVENT_CLICKED) {
        return;
    }

    sensor_index = pressure_ui_find_sensor_index_from_obj(obj);
    if(sensor_index >= 0) {
        pressure_ui_open_detail((uint8_t)sensor_index);
    }
}

static void pressure_ui_cal_btn_event_cb(lv_obj_t *obj, lv_event_t event)
{
    pressure_ui_card_event_cb(obj, event);
}

static void pressure_ui_capture_btn_event_cb(lv_obj_t *obj, lv_event_t event)
{
    int32_t point_index;
    int32_t reference_x100;

    if((event != LV_EVENT_CLICKED) || (s_pressure_detail_sensor >= pressure_manager_get_sensor_count())) {
        return;
    }

    point_index = pressure_ui_find_point_index_from_obj(obj);
    if(point_index < 0) {
        return;
    }

    if(pressure_ui_parse_mmhg_x100(lv_textarea_get_text(s_pressure_point_rows[point_index].ref_ta), &reference_x100) == false) {
        lv_label_set_text(s_pressure_detail_status, "Invalid reference input");
        return;
    }

    if(pressure_manager_capture_calibration_point(s_pressure_detail_sensor, (uint8_t)point_index, reference_x100) == false) {
        lv_label_set_text(s_pressure_detail_status, "Capture failed, sensor not ready");
        return;
    }

    pressure_ui_destroy_keyboard();
    pressure_ui_load_detail_points();
}

static void pressure_ui_save_point_btn_event_cb(lv_obj_t *obj, lv_event_t event)
{
    pressure_manager_calibration_t calibration;
    pressure_manager_data_t pressure_data;
    int32_t point_index;
    int32_t reference_x100;
    uint32_t filtered_counts = 0U;

    if((event != LV_EVENT_CLICKED) || (s_pressure_detail_sensor >= pressure_manager_get_sensor_count())) {
        return;
    }

    point_index = pressure_ui_find_point_index_from_obj(obj);
    if(point_index < 0) {
        return;
    }

    if(pressure_ui_parse_mmhg_x100(lv_textarea_get_text(s_pressure_point_rows[point_index].ref_ta), &reference_x100) == false) {
        lv_label_set_text(s_pressure_detail_status, "Invalid standard value");
        return;
    }

    if(pressure_manager_get_calibration(s_pressure_detail_sensor, &calibration) &&
       calibration.points[point_index].valid) {
        filtered_counts = calibration.points[point_index].filtered_counts;
    } else if(pressure_manager_get_data(s_pressure_detail_sensor, &pressure_data) && pressure_data.data_valid) {
        filtered_counts = pressure_data.filtered_counts;
    }

    if(pressure_manager_set_calibration_point(s_pressure_detail_sensor,
                                              (uint8_t)point_index,
                                              reference_x100,
                                              filtered_counts) == false) {
        lv_label_set_text(s_pressure_detail_status, "Save failed");
        return;
    }

    pressure_ui_destroy_keyboard();
    pressure_ui_load_detail_points();
}

static void pressure_ui_clear_point_btn_event_cb(lv_obj_t *obj, lv_event_t event)
{
    int32_t point_index;

    if((event != LV_EVENT_CLICKED) || (s_pressure_detail_sensor >= pressure_manager_get_sensor_count())) {
        return;
    }

    point_index = pressure_ui_find_point_index_from_obj(obj);
    if(point_index < 0) {
        return;
    }

    if(pressure_manager_clear_calibration_point(s_pressure_detail_sensor, (uint8_t)point_index)) {
        pressure_ui_load_detail_points();
    }
}

static void pressure_ui_detail_action_event_cb(lv_obj_t *obj, lv_event_t event)
{
    if(event != LV_EVENT_CLICKED) {
        return;
    }

    if((obj == s_pressure_detail) || (obj == s_pressure_detail_close_button)) {
        pressure_ui_close_detail();
    } else if((obj == s_pressure_detail_clear_button) && (s_pressure_detail_sensor < pressure_manager_get_sensor_count())) {
        if(pressure_manager_clear_calibration(s_pressure_detail_sensor)) {
            pressure_ui_load_detail_points();
        }
    }
}

static void pressure_ui_init(void)
{
    lv_obj_t *page_scrl;
    lv_obj_t *obj;
    lv_obj_t *label;
    lv_coord_t y;
    uint32_t i;

    if(s_pressure_root != NULL) {
        return;
    }

    s_pressure_root = lv_cont_create(lv_layer_top(), NULL);
    lv_obj_set_size(s_pressure_root, 480, 272);
    lv_obj_align(s_pressure_root, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_local_bg_color(s_pressure_root, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x14, 0x1A, 0x22));
    lv_obj_set_style_local_bg_opa(s_pressure_root, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_border_width(s_pressure_root, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_all(s_pressure_root, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

    s_pressure_header_label = lv_label_create(s_pressure_root, NULL);
    lv_label_set_text_static(s_pressure_header_label, "Pressure Sensors");
    lv_obj_set_style_local_text_color(s_pressure_header_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_obj_set_style_local_text_font(s_pressure_header_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);
    lv_obj_align(s_pressure_header_label, s_pressure_root, LV_ALIGN_IN_TOP_LEFT, 12, 10);

    s_pressure_count_label = lv_label_create(s_pressure_root, NULL);
    lv_label_set_text_static(s_pressure_count_label, "0 detected");
    lv_obj_set_style_local_text_color(s_pressure_count_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_SILVER);
    lv_obj_align(s_pressure_count_label, s_pressure_root, LV_ALIGN_IN_TOP_RIGHT, -12, 14);

    s_pressure_page = lv_page_create(s_pressure_root, NULL);
    lv_obj_set_size(s_pressure_page, 468, 224);
    lv_obj_align(s_pressure_page, s_pressure_root, LV_ALIGN_IN_BOTTOM_MID, 0, -6);
    lv_page_set_scrollbar_mode(s_pressure_page, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_local_bg_color(s_pressure_page, LV_PAGE_PART_BG, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x1B, 0x22, 0x2C));
    lv_obj_set_style_local_bg_opa(s_pressure_page, LV_PAGE_PART_BG, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_border_width(s_pressure_page, LV_PAGE_PART_BG, LV_STATE_DEFAULT, 0);

    page_scrl = lv_page_get_scrl(s_pressure_page);

    s_pressure_empty_label = lv_label_create(page_scrl, NULL);
    lv_label_set_text_static(s_pressure_empty_label, "No pressure sensor detected");
    lv_obj_set_style_local_text_color(s_pressure_empty_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_SILVER);
    lv_obj_align(s_pressure_empty_label, page_scrl, LV_ALIGN_IN_TOP_MID, 0, 20);

    for(i = 0U; i < PRESSURE_MANAGER_MAX_SENSORS; i++) {
        s_pressure_cards[i].card = lv_cont_create(page_scrl, NULL);
        lv_obj_set_size(s_pressure_cards[i].card, 444, PRESSURE_UI_CARD_HEIGHT);
        lv_obj_set_pos(s_pressure_cards[i].card, 8, 8 + (lv_coord_t)i * (PRESSURE_UI_CARD_HEIGHT + PRESSURE_UI_CARD_GAP));
        lv_obj_set_event_cb(s_pressure_cards[i].card, pressure_ui_card_event_cb);
        lv_obj_set_style_local_radius(s_pressure_cards[i].card, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 6);
        lv_obj_set_style_local_bg_color(s_pressure_cards[i].card, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x2A, 0x33, 0x40));
        lv_obj_set_style_local_bg_opa(s_pressure_cards[i].card, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
        lv_obj_set_style_local_border_width(s_pressure_cards[i].card, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

        s_pressure_cards[i].title_label = lv_label_create(s_pressure_cards[i].card, NULL);
        lv_obj_set_style_local_text_color(s_pressure_cards[i].title_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_obj_set_pos(s_pressure_cards[i].title_label, 10, 6);

        s_pressure_cards[i].value_label = lv_label_create(s_pressure_cards[i].card, NULL);
        lv_obj_set_style_local_text_color(s_pressure_cards[i].value_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x7E, 0xE7, 0xAE));
        lv_obj_set_style_local_text_font(s_pressure_cards[i].value_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);
        lv_obj_set_pos(s_pressure_cards[i].value_label, 10, 26);

        s_pressure_cards[i].route_label = lv_label_create(s_pressure_cards[i].card, NULL);
        lv_obj_set_style_local_text_color(s_pressure_cards[i].route_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_SILVER);
        lv_obj_set_pos(s_pressure_cards[i].route_label, 160, 6);

        s_pressure_cards[i].status_label = lv_label_create(s_pressure_cards[i].card, NULL);
        lv_obj_set_width(s_pressure_cards[i].status_label, 170);
        lv_obj_set_style_local_text_color(s_pressure_cards[i].status_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_obj_set_pos(s_pressure_cards[i].status_label, 160, 26);

        s_pressure_cards[i].calibration_label = lv_label_create(s_pressure_cards[i].card, NULL);
        lv_obj_set_style_local_text_color(s_pressure_cards[i].calibration_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0xF2, 0xC2, 0x4D));
        lv_obj_set_pos(s_pressure_cards[i].calibration_label, 340, 10);

        s_pressure_cards[i].calibration_button = lv_btn_create(s_pressure_cards[i].card, NULL);
        lv_obj_set_size(s_pressure_cards[i].calibration_button, 70, 30);
        lv_obj_align(s_pressure_cards[i].calibration_button, s_pressure_cards[i].card, LV_ALIGN_IN_RIGHT_MID, -8, 0);
        lv_obj_set_event_cb(s_pressure_cards[i].calibration_button, pressure_ui_cal_btn_event_cb);

        lv_obj_set_hidden(s_pressure_cards[i].calibration_button, true);
    }

    s_pressure_detail = lv_cont_create(s_pressure_root, NULL);
    lv_obj_set_size(s_pressure_detail, 468, 260);
    lv_obj_align(s_pressure_detail, s_pressure_root, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_event_cb(s_pressure_detail, pressure_ui_detail_action_event_cb);
    lv_obj_set_style_local_bg_color(s_pressure_detail, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x0E, 0x13, 0x19));
    lv_obj_set_style_local_bg_opa(s_pressure_detail, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_90);
    lv_obj_set_style_local_border_width(s_pressure_detail, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_radius(s_pressure_detail, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 8);
    lv_obj_set_hidden(s_pressure_detail, true);

    s_pressure_detail_title = lv_label_create(s_pressure_detail, NULL);
    lv_obj_set_style_local_text_color(s_pressure_detail_title, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_obj_set_style_local_text_font(s_pressure_detail_title, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);
    lv_obj_set_pos(s_pressure_detail_title, 12, 10);

    s_pressure_detail_value = lv_label_create(s_pressure_detail, NULL);
    lv_obj_set_style_local_text_color(s_pressure_detail_value, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x7E, 0xE7, 0xAE));
    lv_obj_set_style_local_text_font(s_pressure_detail_value, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);
    lv_obj_set_pos(s_pressure_detail_value, 12, 34);

    s_pressure_detail_status = lv_label_create(s_pressure_detail, NULL);
    lv_obj_set_width(s_pressure_detail_status, 440);
    lv_obj_set_style_local_text_color(s_pressure_detail_status, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_SILVER);
    lv_obj_set_pos(s_pressure_detail_status, 12, 60);

    label = lv_label_create(s_pressure_detail, NULL);
    lv_label_set_text_static(label, "Std");
    lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_SILVER);
    lv_obj_set_pos(label, 44, 86);

    label = lv_label_create(s_pressure_detail, NULL);
    lv_label_set_text_static(label, "Meas");
    lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_SILVER);
    lv_obj_set_pos(label, 126, 86);

    label = lv_label_create(s_pressure_detail, NULL);
    lv_label_set_text_static(label, "Raw");
    lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_SILVER);
    lv_obj_set_pos(label, 382, 86);

    y = 88;
    for(i = 0U; i < PRESSURE_MANAGER_MAX_CAL_POINTS; i++) {
        label = lv_label_create(s_pressure_detail, NULL);
        {
            char point_name[8];
            snprintf(point_name, sizeof(point_name), "P%u", (unsigned int)(i + 1U));
            lv_label_set_text(label, point_name);
        }
        lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_obj_set_pos(label, 12, y + 7);

        s_pressure_point_rows[i].ref_ta = lv_textarea_create(s_pressure_detail, NULL);
        lv_obj_set_size(s_pressure_point_rows[i].ref_ta, 74, 28);
        lv_obj_set_pos(s_pressure_point_rows[i].ref_ta, 34, y);
        lv_textarea_set_one_line(s_pressure_point_rows[i].ref_ta, true);
        lv_textarea_set_max_length(s_pressure_point_rows[i].ref_ta, 9);
        lv_obj_set_click(s_pressure_point_rows[i].ref_ta, false);

        s_pressure_point_rows[i].measured_ta = lv_textarea_create(s_pressure_detail, NULL);
        lv_obj_set_size(s_pressure_point_rows[i].measured_ta, 74, 28);
        lv_obj_set_pos(s_pressure_point_rows[i].measured_ta, 114, y);
        lv_textarea_set_one_line(s_pressure_point_rows[i].measured_ta, true);
        lv_textarea_set_max_length(s_pressure_point_rows[i].measured_ta, 9);
        lv_obj_set_click(s_pressure_point_rows[i].measured_ta, false);

        s_pressure_point_rows[i].capture_button = lv_btn_create(s_pressure_detail, NULL);
        lv_obj_set_size(s_pressure_point_rows[i].capture_button, 48, 28);
        lv_obj_set_pos(s_pressure_point_rows[i].capture_button, 194, y);
        lv_obj_set_event_cb(s_pressure_point_rows[i].capture_button, pressure_ui_capture_btn_event_cb);
        lv_obj_set_hidden(s_pressure_point_rows[i].capture_button, true);
        obj = lv_label_create(s_pressure_point_rows[i].capture_button, NULL);
        lv_label_set_text_static(obj, "Cap");
        lv_obj_align(obj, s_pressure_point_rows[i].capture_button, LV_ALIGN_CENTER, 0, 0);

        s_pressure_point_rows[i].save_button = lv_btn_create(s_pressure_detail, NULL);
        lv_obj_set_size(s_pressure_point_rows[i].save_button, 48, 28);
        lv_obj_set_pos(s_pressure_point_rows[i].save_button, 246, y);
        lv_obj_set_event_cb(s_pressure_point_rows[i].save_button, pressure_ui_save_point_btn_event_cb);
        lv_obj_set_hidden(s_pressure_point_rows[i].save_button, true);
        obj = lv_label_create(s_pressure_point_rows[i].save_button, NULL);
        lv_label_set_text_static(obj, "Save");
        lv_obj_align(obj, s_pressure_point_rows[i].save_button, LV_ALIGN_CENTER, 0, 0);

        s_pressure_point_rows[i].clear_button = lv_btn_create(s_pressure_detail, NULL);
        lv_obj_set_size(s_pressure_point_rows[i].clear_button, 48, 28);
        lv_obj_set_pos(s_pressure_point_rows[i].clear_button, 298, y);
        lv_obj_set_event_cb(s_pressure_point_rows[i].clear_button, pressure_ui_clear_point_btn_event_cb);
        lv_obj_set_hidden(s_pressure_point_rows[i].clear_button, true);
        obj = lv_label_create(s_pressure_point_rows[i].clear_button, NULL);
        lv_label_set_text_static(obj, "Del");
        lv_obj_align(obj, s_pressure_point_rows[i].clear_button, LV_ALIGN_CENTER, 0, 0);

        s_pressure_point_rows[i].raw_label = lv_label_create(s_pressure_detail, NULL);
        lv_obj_set_width(s_pressure_point_rows[i].raw_label, 108);
        lv_obj_set_style_local_text_color(s_pressure_point_rows[i].raw_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_obj_set_pos(s_pressure_point_rows[i].raw_label, 352, y + 7);
        lv_label_set_text_static(s_pressure_point_rows[i].raw_label, "Raw --");

        y += 32;
    }

    s_pressure_detail_clear_button = lv_btn_create(s_pressure_detail, NULL);
    lv_obj_set_size(s_pressure_detail_clear_button, 96, 30);
    lv_obj_set_pos(s_pressure_detail_clear_button, 248, 224);
    lv_obj_set_event_cb(s_pressure_detail_clear_button, pressure_ui_detail_action_event_cb);
    lv_obj_set_hidden(s_pressure_detail_clear_button, true);
    label = lv_label_create(s_pressure_detail_clear_button, NULL);
    lv_label_set_text_static(label, "Clear All");
    lv_obj_align(label, s_pressure_detail_clear_button, LV_ALIGN_CENTER, 0, 0);

    s_pressure_detail_close_button = lv_btn_create(s_pressure_detail, NULL);
    lv_obj_set_size(s_pressure_detail_close_button, 78, 28);
    lv_obj_set_pos(s_pressure_detail_close_button, 378, 8);
    lv_obj_set_event_cb(s_pressure_detail_close_button, pressure_ui_detail_action_event_cb);
    lv_obj_set_hidden(s_pressure_detail_close_button, true);
    label = lv_label_create(s_pressure_detail_close_button, NULL);
    lv_label_set_text_static(label, "Back");
    lv_obj_align(label, s_pressure_detail_close_button, LV_ALIGN_CENTER, 0, 0);

    pressure_ui_update(true);
}

#if 0
static void pressure_ui_update(bool force)
{
    pressure_manager_data_t pressure_data;
    pressure_manager_route_info_t route_info;
    lv_obj_t *page_scrl;
    uint32_t now;
    uint32_t sensor_count;
    uint32_t i;
    lv_coord_t content_height;
    char text[48];

    if((s_pressure_root == NULL) || (s_pressure_page == NULL) || (s_pressure_count_label == NULL)) {
        return;
    }

    now = swm_gettick();
    if((force == false) && ((now - s_pressure_display_tick) < 500U)) {
        return;
    }
    s_pressure_display_tick = now;

    sensor_count = pressure_manager_get_sensor_count();
    snprintf(text, sizeof(text), "%u detected", (unsigned int)sensor_count);
    lv_label_set_text(s_pressure_count_label, text);
    lv_obj_set_hidden(s_pressure_empty_label, (sensor_count != 0U));

    page_scrl = lv_page_get_scrl(s_pressure_page);
    content_height = 36;
    if(sensor_count > 0U) {
        content_height = 8 + (lv_coord_t)sensor_count * (PRESSURE_UI_CARD_HEIGHT + PRESSURE_UI_CARD_GAP);
    }
    lv_obj_set_size(page_scrl, lv_obj_get_width(page_scrl), content_height);

    if((pressure_manager_get_data(0, &pressure_data) == true) &&
       pressure_data.online &&
       pressure_data.data_valid) {
        if(pressure_data.zero_calibrated) {
            pressure_display_format_fixed(value_text, sizeof(value_text), pressure_data.pressure_mmhg, "mmHg");
            pressure_display_format_fixed(status_text, sizeof(status_text), pressure_data.pressure_kpa, "kPa");
        } else {
            snprintf(value_text, sizeof(value_text), "zeroing...");
            snprintf(status_text, sizeof(status_text), "baseline %u/%u",
                     (unsigned int)pressure_data.zero_sample_count,
                     (unsigned int)PRESSURE_MANAGER_ZERO_CALIBRATION_SAMPLES);
        }

        /* UI 里的 ADC 显示原始传感器计数，避免和补偿后的 raw_counts 混淆。 */
        snprintf(adc_text, sizeof(adc_text), "Raw:%lu Zero:%lu",
                 (unsigned long)pressure_data.sensor_raw_counts,
                 (unsigned long)pressure_data.zero_reference_counts);
    } else if((pressure_manager_get_data(0, &pressure_data) == true) && pressure_data.busy) {
        snprintf(value_text, sizeof(value_text), "reading...");
        snprintf(status_text, sizeof(status_text), "sensor busy");
        snprintf(adc_text, sizeof(adc_text), "Raw:%lu Zero:%lu",
                 (unsigned long)pressure_data.sensor_raw_counts,
                 (unsigned long)pressure_data.zero_reference_counts);
    } else if(pressure_manager_get_data(0, &pressure_data) == true) {
        snprintf(value_text, sizeof(value_text), "offline");
        snprintf(status_text, sizeof(status_text), "err=%u", (unsigned int)pressure_data.last_error);
        snprintf(adc_text, sizeof(adc_text), "Raw:%lu Zero:%lu",
                 (unsigned long)pressure_data.sensor_raw_counts,
                 (unsigned long)pressure_data.zero_reference_counts);
    } else {
        snprintf(value_text, sizeof(value_text), "unavailable");
        snprintf(status_text, sizeof(status_text), "no data");
        snprintf(adc_text, sizeof(adc_text), "Raw:-- Zero:--");
    }

    lv_label_set_text(s_pressure_value_label, value_text);
    lv_label_set_text(s_pressure_status_label, status_text);
    lv_label_set_text(s_pressure_adc_label, adc_text);
}

static void pressure_debug_update(bool force)
{
    pressure_manager_data_t pressure_data;
    uint32_t now;

    now = swm_gettick();
    if((force == false) && ((now - s_pressure_debug_tick) < 1000U)) {
        return;
    }
    s_pressure_debug_tick = now;

    if(pressure_manager_get_data(0, &pressure_data) == false) {
        printf("[pressure] no data object\r\n");
        return;
    }

    printf("[pressure] online=%u busy=%u valid=%u zero=%u zcnt=%u err=%u status=0x%02X sensor_raw=%lu adc=%lu filt=%lu zref=%lu mmHg=%ld.%02ld kPa=%ld.%02ld\r\n",
           pressure_data.online ? 1U : 0U,
           pressure_data.busy ? 1U : 0U,
           pressure_data.data_valid ? 1U : 0U,
           pressure_data.zero_calibrated ? 1U : 0U,
           (unsigned int)pressure_data.zero_sample_count,
           (unsigned int)pressure_data.last_error,
           (unsigned int)pressure_data.status,
           (unsigned long)pressure_data.sensor_raw_counts,
           (unsigned long)pressure_data.raw_counts,
           (unsigned long)pressure_data.filtered_counts,
           (unsigned long)pressure_data.zero_reference_counts,
           (long)(pressure_data.pressure_mmhg),
           (long)((int32_t)(pressure_data.pressure_mmhg * 100.0f) % 100 + (((int32_t)(pressure_data.pressure_mmhg * 100.0f) % 100) < 0 ? 100 : 0)),
           (long)(pressure_data.pressure_kpa),
           (long)((int32_t)(pressure_data.pressure_kpa * 100.0f) % 100 + (((int32_t)(pressure_data.pressure_kpa * 100.0f) % 100) < 0 ? 100 : 0)));
}

static void pressure_i2c_scan_once(void)
{
#if defined(SWM34SVET6_A3)
    uint8_t addr;
    bool found = false;

    if(s_pressure_i2c_scan_done) {
        return;
    }
    s_pressure_i2c_scan_done = true;

    printf("[pressure] scan i2c1 on PA6/PA7, expecting sensor at 0x18\r\n");
    for(addr = 0x08U; addr < 0x78U; addr++) {
        if(dev_i2c1_sensor_probe(addr)) {
            printf("[pressure] i2c1 ack at 0x%02X\r\n", (unsigned int)addr);
            found = true;
        }
    }

    if(found == false) {
        printf("[pressure] i2c1 scan result: no device ack\r\n");
    }
#endif
}
#endif

static void pressure_ui_update(bool force)
{
    pressure_manager_data_t pressure_data;
    pressure_manager_route_info_t route_info;
    lv_obj_t *page_scrl;
    uint32_t now;
    uint32_t sensor_count;
    uint32_t i;
    lv_coord_t content_height;
    char text[48];

    if((s_pressure_root == NULL) || (s_pressure_page == NULL) || (s_pressure_count_label == NULL)) {
        return;
    }

    now = swm_gettick();
    if((force == false) && ((now - s_pressure_display_tick) < 500U)) {
        return;
    }
    s_pressure_display_tick = now;

    sensor_count = pressure_manager_get_sensor_count();
    snprintf(text, sizeof(text), "%u detected", (unsigned int)sensor_count);
    lv_label_set_text(s_pressure_count_label, text);
    lv_obj_set_hidden(s_pressure_empty_label, (sensor_count != 0U));

    page_scrl = lv_page_get_scrl(s_pressure_page);
    content_height = 36;
    if(sensor_count > 0U) {
        content_height = 8 + (lv_coord_t)sensor_count * (PRESSURE_UI_CARD_HEIGHT + PRESSURE_UI_CARD_GAP);
    }
    lv_obj_set_size(page_scrl, lv_obj_get_width(page_scrl), content_height);

    for(i = 0U; i < PRESSURE_MANAGER_MAX_SENSORS; i++) {
        pressure_ui_card_t *card = &s_pressure_cards[i];

        if(i >= sensor_count) {
            lv_obj_set_hidden(card->card, true);
            continue;
        }

        lv_obj_set_hidden(card->card, false);
        lv_obj_set_pos(card->card, 8, 8 + (lv_coord_t)i * (PRESSURE_UI_CARD_HEIGHT + PRESSURE_UI_CARD_GAP));

        snprintf(text, sizeof(text), "Sensor %u", (unsigned int)(i + 1U));
        lv_label_set_text(card->title_label, text);

        if(pressure_manager_get_route_info((uint8_t)i, &route_info)) {
            char route_text[32];

            pressure_ui_route_text(&route_info, route_text, sizeof(route_text));
            lv_label_set_text(card->route_label, route_text);
        } else {
            lv_label_set_text(card->route_label, "Route --");
        }

        if(pressure_manager_get_data((uint8_t)i, &pressure_data) == false) {
            lv_label_set_text(card->value_label, "Unavailable");
            lv_label_set_text(card->status_label, "No runtime data");
            lv_label_set_text(card->calibration_label, "Pts 0/5");
            continue;
        }

        snprintf(text, sizeof(text), "Pts %u/5", (unsigned int)pressure_data.calibration_point_count);
        lv_label_set_text(card->calibration_label, text);

        if(pressure_data.data_valid && pressure_data.online && pressure_data.zero_calibrated) {
            char value_card_text[32];
            char measured_text[32];
            char status_card_text[96];

            pressure_ui_format_mmhg(value_card_text, sizeof(value_card_text), pressure_data.pressure_mmhg, "mmHg");
            pressure_ui_format_mmhg(measured_text, sizeof(measured_text), pressure_data.measured_pressure_mmhg, "mmHg");
            snprintf(status_card_text, sizeof(status_card_text), "Raw %lu  %s",
                     (unsigned long)pressure_data.filtered_counts,
                     measured_text);
            lv_label_set_text(card->value_label, value_card_text);
            lv_label_set_text(card->status_label, status_card_text);
        } else if(pressure_data.data_valid && pressure_data.online) {
            char status_card_text[64];

            lv_label_set_text(card->value_label, "Zeroing");
            snprintf(status_card_text, sizeof(status_card_text), "Baseline %u/%u",
                     (unsigned int)pressure_data.zero_sample_count,
                     (unsigned int)PRESSURE_MANAGER_ZERO_CALIBRATION_SAMPLES);
            lv_label_set_text(card->status_label, status_card_text);
        } else if(pressure_data.busy) {
            lv_label_set_text(card->value_label, "Reading");
            lv_label_set_text(card->status_label, "Sensor busy");
        } else {
            char status_card_text[64];

            lv_label_set_text(card->value_label, "Offline");
            snprintf(status_card_text, sizeof(status_card_text), "Err %u Raw %lu",
                     (unsigned int)pressure_data.last_error,
                     (unsigned long)pressure_data.filtered_counts);
            lv_label_set_text(card->status_label, status_card_text);
        }
    }

    if((s_pressure_detail_sensor != 0xFFU) && (s_pressure_detail_sensor >= sensor_count)) {
        pressure_ui_close_detail();
    } else {
        pressure_ui_refresh_detail_live();
    }
}

static void pressure_debug_update(bool force)
{
    pressure_manager_data_t pressure_data;
    pressure_manager_route_info_t route_info;
    uint32_t now;
    uint32_t sensor_count;
    uint32_t i;
    char route_text[32];

    now = swm_gettick();
    if((force == false) && ((now - s_pressure_debug_tick) < 1000U)) {
        return;
    }
    s_pressure_debug_tick = now;

    sensor_count = pressure_manager_get_sensor_count();
    printf("[pressure] sensors=%u\r\n", (unsigned int)sensor_count);

    if(sensor_count == 0U) {
        printf("[pressure] no route detected\r\n");
        return;
    }

    for(i = 0U; i < sensor_count; i++) {
        if(pressure_manager_get_data((uint8_t)i, &pressure_data) == false) {
            printf("[pressure] [%u] no data object\r\n", (unsigned int)i);
            continue;
        }

        if(pressure_manager_get_route_info((uint8_t)i, &route_info)) {
            pressure_ui_route_text(&route_info, route_text, sizeof(route_text));
        } else {
            snprintf(route_text, sizeof(route_text), "Route --");
        }

        printf("[pressure] [%u] %s online=%u busy=%u valid=%u zero=%u cal=%u raw=%lu filt=%lu zref=%lu meas=%ld.%02ld out=%ld.%02ld err=%u\r\n",
               (unsigned int)i,
               route_text,
               pressure_data.online ? 1U : 0U,
               pressure_data.busy ? 1U : 0U,
               pressure_data.data_valid ? 1U : 0U,
               pressure_data.zero_calibrated ? 1U : 0U,
               (unsigned int)pressure_data.calibration_point_count,
               (unsigned long)pressure_data.sensor_raw_counts,
               (unsigned long)pressure_data.filtered_counts,
               (unsigned long)pressure_data.zero_reference_counts,
               (long)(pressure_data.measured_pressure_mmhg),
               (long)(abs((int32_t)(pressure_data.measured_pressure_mmhg * 100.0f)) % 100),
               (long)(pressure_data.pressure_mmhg),
               (long)(abs((int32_t)(pressure_data.pressure_mmhg * 100.0f)) % 100),
               (unsigned int)pressure_data.last_error);
    }
}

static void pressure_i2c_scan_once(void)
{
#if defined(SWM34SVET6_A3)
    uint8_t addr;
    bool found = false;

    if(s_pressure_i2c_scan_done) {
        return;
    }
    s_pressure_i2c_scan_done = true;

    printf("[pressure] scan i2c1 on PA6/PA7, watching 0x18 / 0x70 / 0x71\r\n");
    for(addr = 0x08U; addr < 0x78U; addr++) {
        if(dev_i2c1_sensor_probe(addr)) {
            printf("[pressure] i2c1 ack at 0x%02X\r\n", (unsigned int)addr);
            found = true;
        }
    }

    if(found == false) {
        printf("[pressure] i2c1 scan result: no device ack\r\n");
    }
#endif
}

static void soft_uart_probe_prepare_line(const struct soft_uart_probe_desc *desc)
{
    /* Re-apply GPIO mode on every round so later peripheral init cannot leave the pin mux elsewhere. */
    PORT_Init(desc->tx_port, desc->tx_pin, 0, 1);
    GPIO_Init(desc->tx_gpio, desc->tx_pin, 1, 1, 0, 0);
    GPIO_SetBit(desc->tx_gpio, desc->tx_pin);

    PORT_Init(desc->rx_port, desc->rx_pin, 0, 1);
    GPIO_Init(desc->rx_gpio, desc->rx_pin, 0, 1, 0, 0);
}

static void soft_uart_probe_send_frame(const struct soft_uart_probe_desc *desc, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint32_t bit;
    uint8_t byte;

    __disable_irq();
    GPIO_SetBit(desc->tx_gpio, desc->tx_pin);
    swm_delay_us(SOFT_UART_PROBE_BIT_US);

    for(i = 0U; i < len; i++) {
        byte = data[i];

        GPIO_ClrBit(desc->tx_gpio, desc->tx_pin);
        swm_delay_us(SOFT_UART_PROBE_BIT_US);

        for(bit = 0U; bit < 8U; bit++) {
            if(byte & 0x01U) {
                GPIO_SetBit(desc->tx_gpio, desc->tx_pin);
            } else {
                GPIO_ClrBit(desc->tx_gpio, desc->tx_pin);
            }

            swm_delay_us(SOFT_UART_PROBE_BIT_US);
            byte >>= 1;
        }

        GPIO_SetBit(desc->tx_gpio, desc->tx_pin);
        swm_delay_us(SOFT_UART_PROBE_BIT_US);
    }
    __enable_irq();
}

static bool soft_uart_probe_receive_byte(const struct soft_uart_probe_desc *desc, uint8_t *data, uint32_t timeout_ms)
{
    uint32_t start_tick;
    uint32_t bit;
    uint8_t value;
    bool stop_ok;

    start_tick = swm_gettick();
    while((swm_gettick() - start_tick) < timeout_ms) {
        if(GPIO_GetBit(desc->rx_gpio, desc->rx_pin) == 0U) {
            swm_delay_us(SOFT_UART_PROBE_START_GUARD_US);
            if(GPIO_GetBit(desc->rx_gpio, desc->rx_pin) != 0U) {
                continue;
            }

            __disable_irq();
            swm_delay_us(SOFT_UART_PROBE_BIT_US + SOFT_UART_PROBE_START_GUARD_US);

            value = 0U;
            for(bit = 0U; bit < 8U; bit++) {
                if(GPIO_GetBit(desc->rx_gpio, desc->rx_pin) != 0U) {
                    value |= (uint8_t)(1U << bit);
                }
                swm_delay_us(SOFT_UART_PROBE_BIT_US);
            }

            stop_ok = (GPIO_GetBit(desc->rx_gpio, desc->rx_pin) != 0U);
            __enable_irq();

            if(stop_ok) {
                *data = value;
                return true;
            }
        }

        swm_delay_us(SOFT_UART_PROBE_POLL_US);
    }

    return false;
}

static uint32_t soft_uart_probe_receive_window(const struct soft_uart_probe_desc *desc, uint8_t *buffer, uint32_t max_len, uint32_t window_ms)
{
    uint32_t start_tick;
    uint32_t elapsed;
    uint32_t remain_ms;
    uint32_t rx_len;
    uint8_t rx_byte;

    start_tick = swm_gettick();
    rx_len = 0U;

    while(((swm_gettick() - start_tick) < window_ms) && (rx_len < max_len)) {
        elapsed = swm_gettick() - start_tick;
        remain_ms = (elapsed < window_ms) ? (window_ms - elapsed) : 0U;
        if((remain_ms == 0U) || (soft_uart_probe_receive_byte(desc, &rx_byte, remain_ms) == false)) {
            break;
        }

        buffer[rx_len++] = rx_byte;
    }

    return rx_len;
}

static void soft_uart_probe_log_rx(const uint8_t *buffer, uint32_t len)
{
    uint32_t i;

    printf("hex=");
    for(i = 0U; i < len; i++) {
        printf("%02X", (unsigned int)buffer[i]);
        if(i + 1U < len) {
            printf(" ");
        }
    }

    printf(" ascii=\"");
    for(i = 0U; i < len; i++) {
        if((buffer[i] >= 32U) && (buffer[i] <= 126U)) {
            printf("%c", buffer[i]);
        } else {
            printf(".");
        }
    }
    printf("\"");
}

static void soft_uart_probe_init(void)
{
    uint32_t i;

    if(s_soft_uart_probe_inited) {
        return;
    }
    s_soft_uart_probe_inited = true;
    s_soft_uart_probe_tick = swm_gettick();

    printf("[uart-probe] start gpio soft-uart probe, using configured tx/rx pairs\r\n");
    printf("[uart-probe] round_period=%lu ms rx_window=%lu ms bit_time=%lu us (~9600bps)\r\n",
           (unsigned long)SOFT_UART_PROBE_PERIOD_MS,
           (unsigned long)SOFT_UART_PROBE_RX_WINDOW_MS,
           (unsigned long)SOFT_UART_PROBE_BIT_US);

    for(i = 0U; i < (uint32_t)(sizeof(s_soft_uart_probe_descs) / sizeof(s_soft_uart_probe_descs[0])); i++) {
        printf("[uart-probe] %s tx=%s rx=%s note=%s\r\n",
               s_soft_uart_probe_descs[i].uart_name,
               s_soft_uart_probe_descs[i].tx_name,
               s_soft_uart_probe_descs[i].rx_name,
               s_soft_uart_probe_descs[i].runtime_note);
    }
}

static void soft_uart_probe_poll(void)
{
    const struct soft_uart_probe_desc *desc;
    uint8_t tx_data[2];
    uint8_t rx_buffer[SOFT_UART_PROBE_RX_MAX_LEN];
    uint32_t rx_len;
    uint32_t now;

    if(s_soft_uart_probe_inited == false) {
        return;
    }

    now = swm_gettick();
    if((now - s_soft_uart_probe_tick) < SOFT_UART_PROBE_PERIOD_MS) {
        return;
    }
    s_soft_uart_probe_tick = now;

    desc = &s_soft_uart_probe_descs[s_soft_uart_probe_index];
    tx_data[0] = (uint8_t)'U';
    tx_data[1] = (uint8_t)('1' + s_soft_uart_probe_index);

    s_soft_uart_probe_index++;
    if(s_soft_uart_probe_index >= (uint32_t)(sizeof(s_soft_uart_probe_descs) / sizeof(s_soft_uart_probe_descs[0]))) {
        s_soft_uart_probe_index = 0U;
    }

    soft_uart_probe_prepare_line(desc);

    printf("[uart-probe] %s tx=%s rx=%s send=%c%c\r\n",
           desc->uart_name,
           desc->tx_name,
           desc->rx_name,
           tx_data[0],
           tx_data[1]);

    soft_uart_probe_send_frame(desc, tx_data, sizeof(tx_data));
    rx_len = soft_uart_probe_receive_window(desc, rx_buffer, sizeof(rx_buffer), SOFT_UART_PROBE_RX_WINDOW_MS);

    if(rx_len > 0U) {
        printf("[uart-probe] %s rx_len=%lu ",
               desc->uart_name,
               (unsigned long)rx_len);
        soft_uart_probe_log_rx(rx_buffer, rx_len);
        printf("\r\n");
    } else {
        printf("[uart-probe] %s rx_len=0 no data in rx window\r\n", desc->uart_name);
    }
}

void on_boot(void)
{    
    // 检测和处理U盘模式
    app_usbmsc_handler();
    
    uart_init();
    
    char ver[48];
    synwit_ui_get_platform_version_name(ver, sizeof(ver));
    printf("UI platform version: %s\n", ver);
    
    systick_init();
}

void framework_ready(void)
{
    lv_coord_t hres = synwit_ui_manifest_get_int("peripheral.display.hres", 480);
    lv_coord_t vres = synwit_ui_manifest_get_int("peripheral.display.vres", 272);
    int rotation = synwit_ui_manifest_get_int("peripheral.display.rotation", 0);
    
    lcdc_init();
    peripheral_lcd_init(hres, vres);
    jpeg_init();
    
    //显示开机logo和开机动画
    bootscreen(hres, vres);
    
    // LVGL硬件相关服务初始化
    lv_port_jpeg_init();
    lv_port_disp_init(hres, vres, rotation);
    lv_port_indev_init();
    if(lv_disp_get_hor_res(NULL) >= 1024 ||
        lv_disp_get_ver_res(NULL) >= 1024) {
            _lv_mem_set_deceleration(1);
    }

    // 挂载外部存储器(TF卡、外接U盘)
    mount_external_fs();
    // UI框架准备就绪后，就可以注册各个界面了
    app_register_screens();
    
    // 执行用户自定义的初始化流程
    user_init();
}

void app_ready(void)
{
    lvgl_layer_init();
    
    bootscreen_cleanup();
    pressure_ui_init();
    pressure_display_update(true);  //切换到lvgl显存后，再释放动画显存
}

void main_tick(void)
{
    // 主循环回调函数，LVGL主循环运行过程中，会定时/不定时
    // 的调用这个函数
//    if(判断key状态)
//		{
//					synwit_ui_load_screen(SCREEN002);
//		}
    /* 串口屏功能——处理串口命令及发送应答 */
    if(g_sdisp_ops.rx_handler) {
        g_sdisp_ops.rx_handler();
    }

    pressure_i2c_scan_once();
#if (SOFT_UART_PROBE_ENABLE != 0U)
    soft_uart_probe_poll();
#endif
    pressure_manager_poll();
    pressure_display_update(false);
    pressure_debug_update(false);
}

void on_ezoc_msg(const char *msg, uint32_t len)
{
    /* 串口屏功能——发送通知帧 */
    if(g_sdisp_ops.notify) {
        g_sdisp_ops.notify((const uint8_t *)msg, len);
    }
}

void SysTick_Handler(void)
{
    swm_inctick();
    lv_tick_inc(1);
	
}

static synwit_app_callback_t s_app_ops = {
    .framework_ready = framework_ready,
    .app_ready = app_ready,
    
    // UI数据(ui.bin)访问接口(需与实际存储方式匹配)：
    //     文件系统： data_provider_fs_read
    //     flash直读：data_provider_spiflash_read
    .ui_data_read = data_provider_spiflash_read,
    
    .main_tick = main_tick,
    /* main_tick默认触发间隔为50ms */
    .main_tick_period = 50,
    /* main_tick回调的触发优先级默认为LV_TASK_PRIO_HIGH */
    .main_tick_prio = LV_TASK_PRIO_HIGH,    // LV_TASK_PRIO_HIGHEST
    
    .on_ezoc_msg = on_ezoc_msg,
};

static void mount_external_fs()
{
    static FATFS sd_fatfs;
    
    int enabled_sd = synwit_ui_manifest_get_int("peripheral.storage.sd", 0);
    if(enabled_sd) {
        f_mount(&sd_fatfs, "sd:", 1);
    }
}

static void user_init()
{
    // TODO: 驱动初始化、app全局变量初始化等代码可以添加在这里。
    //（注意：UI对象的创建、加载等不要加在这里，请在对应的screenXXX.c
    // 内进行UI对象的创建及操作）
    pressure_manager_init();
#if (SOFT_UART_PROBE_ENABLE != 0U)
    soft_uart_probe_init();
#endif

}

int main()
{   
    on_boot();

    // 注意! 初始化代码请添加到user_init()函数中而不是这里。因
    // 为执行到这里时，HMI平台还没启动，如果在这里调用任何LVGL
    // 接口，或"synwit_ui_xxxx"类接口，都无法保证能正常执行。
    
    // 启动HMI平台，这个函数会以阻塞模式运行，不会返回。
    synwit_ui_start(MANIFEST_DIR, &s_app_ops);
}
