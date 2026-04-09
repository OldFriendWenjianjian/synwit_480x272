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
static void pressure_display_init(void);
static void pressure_display_update(bool force);
static void pressure_debug_update(bool force);
static void pressure_i2c_scan_once(void);
static void soft_uart_probe_init(void);
static void soft_uart_probe_poll(void);
static void soft_uart_probe_prepare_line(const struct soft_uart_probe_desc *desc);
static void soft_uart_probe_send_frame(const struct soft_uart_probe_desc *desc, const uint8_t *data, uint32_t len);
static bool soft_uart_probe_receive_byte(const struct soft_uart_probe_desc *desc, uint8_t *data, uint32_t timeout_ms);
static uint32_t soft_uart_probe_receive_window(const struct soft_uart_probe_desc *desc, uint8_t *buffer, uint32_t max_len, uint32_t window_ms);
static void soft_uart_probe_log_rx(const uint8_t *buffer, uint32_t len);

static lv_obj_t *s_pressure_panel = NULL;
static lv_obj_t *s_pressure_title_label = NULL;
static lv_obj_t *s_pressure_value_label = NULL;
static lv_obj_t *s_pressure_status_label = NULL;
static lv_obj_t *s_pressure_adc_label = NULL;
static uint32_t s_pressure_display_tick = 0U;
static uint32_t s_pressure_debug_tick = 0U;
static bool s_pressure_i2c_scan_done = false;

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

static void pressure_display_format_fixed(char *buf, size_t size, float value, const char *unit)
{
    int32_t scaled;
    int32_t integer;
    int32_t fraction;

    scaled = (int32_t)(value * 100.0f);
    if(value >= 0.0f) {
        scaled += 0;
    } else {
        scaled -= 0;
    }

    integer = scaled / 100;
    fraction = scaled % 100;
    if(fraction < 0) {
        fraction = -fraction;
    }

    snprintf(buf, size, "%ld.%02ld %s", (long)integer, (long)fraction, unit);
}

static void pressure_display_init(void)
{
    if(s_pressure_value_label != NULL) {
        return;
    }

    s_pressure_panel = lv_obj_create(lv_layer_top(), NULL);
    lv_obj_set_size(s_pressure_panel, 220, 108);
    lv_obj_align(s_pressure_panel, NULL, LV_ALIGN_IN_TOP_LEFT, 8, 8);
    lv_obj_set_click(s_pressure_panel, false);
    lv_obj_set_style_local_radius(s_pressure_panel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 8);
    lv_obj_set_style_local_bg_opa(s_pressure_panel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_70);
    lv_obj_set_style_local_bg_color(s_pressure_panel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_style_local_border_width(s_pressure_panel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_all(s_pressure_panel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 6);

    s_pressure_title_label = lv_label_create(s_pressure_panel, NULL);
    lv_label_set_text_static(s_pressure_title_label, "Pressure Sensor");
    lv_obj_set_click(s_pressure_title_label, false);
    lv_obj_set_style_local_text_color(s_pressure_title_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_obj_set_style_local_text_opa(s_pressure_title_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_text_font(s_pressure_title_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);
    lv_obj_align(s_pressure_title_label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

    s_pressure_value_label = lv_label_create(s_pressure_panel, NULL);
    lv_label_set_text_static(s_pressure_value_label, "--.-- mmHg");
    lv_obj_set_click(s_pressure_value_label, false);
    lv_obj_set_style_local_text_color(s_pressure_value_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_obj_set_style_local_text_opa(s_pressure_value_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_text_font(s_pressure_value_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);
    lv_obj_align(s_pressure_value_label, s_pressure_title_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);

    s_pressure_status_label = lv_label_create(s_pressure_panel, NULL);
    lv_label_set_text_static(s_pressure_status_label, "init");
    lv_obj_set_click(s_pressure_status_label, false);
    lv_obj_set_style_local_text_color(s_pressure_status_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_obj_set_style_local_text_opa(s_pressure_status_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_width(s_pressure_status_label, 206);
    lv_obj_align(s_pressure_status_label, s_pressure_value_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

    s_pressure_adc_label = lv_label_create(s_pressure_panel, NULL);
    lv_label_set_text_static(s_pressure_adc_label, "Raw:-- Zero:--");
    lv_obj_set_click(s_pressure_adc_label, false);
    lv_obj_set_style_local_text_color(s_pressure_adc_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_obj_set_style_local_text_opa(s_pressure_adc_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_width(s_pressure_adc_label, 206);
    lv_obj_align(s_pressure_adc_label, s_pressure_status_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

    pressure_display_update(true);
}

static void pressure_display_update(bool force)
{
    pressure_manager_data_t pressure_data;
    uint32_t now;
    char value_text[48];
    char status_text[48];
    char adc_text[48];

    if((s_pressure_value_label == NULL) || (s_pressure_status_label == NULL) || (s_pressure_adc_label == NULL)) {
        return;
    }

    now = swm_gettick();
    if((force == false) && ((now - s_pressure_display_tick) < 1000U)) {
        return;
    }
    s_pressure_display_tick = now;

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
    pressure_display_init();
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
