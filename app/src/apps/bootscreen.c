#include "main.h"

#include "lvgl/lvgl.h"
#include "jfif_parser.h"
#include "avi_parser.h"
#include "board.h"
#include "lcd/peripheral_lcd.h"

#define TAG "app"

#define UI_VOLUMN   "S:spi:"

#define BOOTLOGO_NAME   "bootlogo.jpg"
#define BOOTANIMATE_NAME   "bootanimate.avi"

static void *splash_buf = NULL; // 开机LOGO/开机动画显存指针

#if 0
static void bootsplash_refr(int to_x1, int to_x2, int to_y1, int to_y2, lv_color_t *fb);

static void bootscreen_fill(lv_color_t *fb, uint32_t count, lv_color_t color)
{
    uint32_t i;

    for(i = 0U; i < count; i++) {
        fb[i] = color;
    }
}

static void bootscreen_fill_rect(lv_color_t *fb,
                                 int width,
                                 int height,
                                 int x,
                                 int y,
                                 int rect_w,
                                 int rect_h,
                                 lv_color_t color)
{
    int row;
    int col;
    int x_start = (x < 0) ? 0 : x;
    int y_start = (y < 0) ? 0 : y;
    int x_end = x + rect_w;
    int y_end = y + rect_h;

    if(x_end > width) {
        x_end = width;
    }
    if(y_end > height) {
        y_end = height;
    }

    for(row = y_start; row < y_end; row++) {
        for(col = x_start; col < x_end; col++) {
            fb[row * width + col] = color;
        }
    }
}

static const uint8_t *bootscreen_get_glyph(char ch)
{
    static const uint8_t glyph_space[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t glyph_O[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    static const uint8_t glyph_P[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
    static const uint8_t glyph_E[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
    static const uint8_t glyph_N[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
    static const uint8_t glyph_H[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    static const uint8_t glyph_A[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    static const uint8_t glyph_R[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
    static const uint8_t glyph_M[7] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
    static const uint8_t glyph_Y[7] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};

    switch(ch) {
    case 'O': return glyph_O;
    case 'P': return glyph_P;
    case 'E': return glyph_E;
    case 'N': return glyph_N;
    case 'H': return glyph_H;
    case 'A': return glyph_A;
    case 'R': return glyph_R;
    case 'M': return glyph_M;
    case 'Y': return glyph_Y;
    case ' ':
    default:
        return glyph_space;
    }
}

static void bootscreen_draw_text(lv_color_t *fb,
                                 int width,
                                 int height,
                                 int x,
                                 int y,
                                 int scale,
                                 const char *text,
                                 lv_color_t color)
{
    int cursor_x = x;

    while((*text != '\0') && (scale > 0)) {
        const uint8_t *glyph = bootscreen_get_glyph(*text);
        int row;
        int col;

        for(row = 0; row < 7; row++) {
            for(col = 0; col < 5; col++) {
                if((glyph[row] & (uint8_t)(1U << (4 - col))) != 0U) {
                    bootscreen_fill_rect(fb,
                                         width,
                                         height,
                                         cursor_x + col * scale,
                                         y + row * scale,
                                         scale,
                                         scale,
                                         color);
                }
            }
        }

        cursor_x += 6 * scale;
        text++;
    }
}

static void bootscreen_show_fullscreen(int hres, int vres)
{
    LCD_LayerInitStructure LCD_layerInitStruct;

    LCD_layerInitStruct.Alpha = 0xFF;
    LCD_layerInitStruct.HStart = 0;
    LCD_layerInitStruct.HStop = hres - 1;
    LCD_layerInitStruct.VStart = 0;
    LCD_layerInitStruct.VStop = vres - 1;
    LCD_layerInitStruct.DataSource = (uint32_t)splash_buf;
    LCD_LayerInit(LCD, LCD_LAYER_1, &LCD_layerInitStruct);

    if(flag_rgb_initialized == 0) {
        flag_rgb_initialized = 1;
        lcdc_start();
    }

    bootsplash_refr(0, hres - 1, 0, vres - 1, (lv_color_t *)splash_buf);
    lcd_backlight_on();
}

static void bootscreen_show_openharmony(int hres, int vres)
{
    lv_color_t bg = LV_COLOR_MAKE(0x09, 0x12, 0x1F);
    lv_color_t accent = LV_COLOR_MAKE(0x2B, 0x7D, 0xFF);
    lv_color_t accent_soft = LV_COLOR_MAKE(0x66, 0xA8, 0xFF);
    lv_color_t text = LV_COLOR_MAKE(0xF3, 0xF8, 0xFF);
    int text_scale = 4;
    int text_width = (int)(strlen("OPENHARMONY")) * 6 * text_scale;
    int text_x = (hres - text_width) / 2;

    bootscreen_fill((lv_color_t *)splash_buf, (uint32_t)(hres * vres), bg);
    bootscreen_fill_rect((lv_color_t *)splash_buf, hres, vres, 64, 42, hres - 128, 8, accent);
    bootscreen_fill_rect((lv_color_t *)splash_buf, hres, vres, 120, 58, hres - 240, 6, accent_soft);
    bootscreen_fill_rect((lv_color_t *)splash_buf, hres, vres, 144, 188, hres - 288, 6, accent_soft);
    bootscreen_fill_rect((lv_color_t *)splash_buf, hres, vres, 88, 204, hres - 176, 8, accent);
    bootscreen_draw_text((lv_color_t *)splash_buf, hres, vres, text_x, 92, text_scale, "OPENHARMONY", text);
    bootscreen_show_fullscreen(hres, vres);
    swm_delay_ms(800);
}

#endif
static void bootsplash_refr(int to_x1, int to_x2, int to_y1, int to_y2, lv_color_t *fb)
{
    if(lcdc_get_drive_mode() == LCDC_DRIVE_MODE_MPU) {
        MPU_LCD.setcur(to_x1, to_x2, to_y1, to_y2);
        for(int y = to_y1; y <= to_y2; y++) {
            for(int x = to_x1; x <= to_x2; x++) {
                LCD_WR_DATA(LCD, (uint16_t)fb->full);
                fb++;
            }
        }
    }
}

void bootscreen(int hres, int vres)
{
    uint16_t x1, x2, y1, y2;
    
    uint8_t *data_buf = lv_mem_alloc(256 * 1024); //jpg解码数据缓冲区
    jfif_info_t *jfif_info = lv_mem_alloc(sizeof(jfif_info_t));
    splash_buf = lv_mem_alloc_align(hres * vres * LV_COLOR_DEPTH / 8, 32);
    if(splash_buf == NULL) {
        return;
    }

    jpeg_outset_t jpeg_outset;
#if  LV_COLOR_DEPTH == 32
    jpeg_outset.format = JPEG_OUT_XRGB888;
#elif  LV_COLOR_DEPTH == 16
    jpeg_outset.format = JPEG_OUT_RGB565;
#endif
    jpeg_outset.dither = 0;
    jpeg_outset.RGBAddr = (uint32_t)splash_buf;
    
    char filepath[256];
    lv_fs_file_t file;
    lv_fs_res_t fs_res;
    uint32_t file_size;
    
    // 有开机图片则使用图片作为开机画面
    snprintf(filepath, sizeof(filepath), "%s/%s", UI_VOLUMN, BOOTLOGO_NAME);
    fs_res = lv_fs_open(&file, filepath, LV_FS_MODE_RD);
    if (fs_res == LV_FS_RES_OK) 
    {
        lv_fs_size(&file, &file_size);
        lv_fs_read(&file, data_buf, file_size, NULL);
        lv_fs_close(&file);
        jfif_parse(data_buf, file_size, jfif_info);
        if((jfif_info->Width <= hres) && (jfif_info->Height <= vres))
        {
            jpeg_decode(JPEG, jfif_info, &jpeg_outset, jfif_info->Width);
            while(JPEG_DecodeBusy(JPEG)) __NOP();
            
            x1 = (hres - jfif_info->Width) / 2;
            x2 = x1 + jfif_info->Width - 1;
            y1 = (vres - jfif_info->Height) / 2;
            y2 = y1 + jfif_info->Height - 1;
            //logo图片居中显示，不支持超过屏幕分辨率的图片
            LCD_LayerInitStructure LCD_layerInitStruct;
            LCD_layerInitStruct.Alpha = 0xFF;
            LCD_layerInitStruct.HStart = x1;
            LCD_layerInitStruct.HStop = x2;
            LCD_layerInitStruct.VStart = y1;
            LCD_layerInitStruct.VStop = y2;
            LCD_layerInitStruct.DataSource = (uint32_t)splash_buf;
            LCD_LayerInit(LCD, LCD_LAYER_1, &LCD_layerInitStruct);
            flag_rgb_initialized = 1;   //LCD_Start只能调用一次，用此标志位标识
            lcdc_start();
            bootsplash_refr(x1, x2, y1, y2, splash_buf);
            lcd_backlight_on();
            swm_delay_ms(1000); //此延时可根据实际需求调整
        }
        else
        {
            printf("Boot logo is too big.\n");
        }
    }
    else
    {
        // 没有开机图片则尝试使用开机视频作为开机画面
        snprintf(filepath, sizeof(filepath), "%s/%s", UI_VOLUMN, BOOTANIMATE_NAME);
        fs_res = lv_fs_open(&file, filepath, LV_FS_MODE_RD);
        if (fs_res == LV_FS_RES_OK) 
        {
            lv_fs_read(&file, data_buf, 10240, NULL);
            
            AVI_INFO *avix = lv_mem_alloc(sizeof(AVI_INFO));
            if (avi_init(avix, (uint8_t *)data_buf, 10240))
            {
                printf("avi err\n");
            }

            if((avix->Width <= hres) && (avix->Height <= vres))
            {
                uint32_t offset = 0;
                offset = avi_srarch_id((uint8_t *)data_buf, 10240, "movi");
                avi_get_streaminfo(avix, (uint8_t *)(data_buf + offset + 4));
                lv_fs_seek(&file, offset + 12, LV_FS_SEEK_SET);
                
                uint32_t t_start,t;
                volatile uint8_t flag_firstframe = 0;
                jpeg_outset.RGBAddr = (uint32_t)splash_buf;
                while (1 == 1)
                {
                    if (avix->StreamID == AVI_VIDS_FLAG) //视频流
                    {
                        t_start = swm_gettick();
                        lv_fs_read(&file, data_buf, avix->StreamSize + 8, NULL);
                        if (avix->StreamSize != 0)
                        {
                            jfif_parse(data_buf, avix->StreamSize, jfif_info);
                            jpeg_decode(JPEG, jfif_info, &jpeg_outset, jfif_info->Width);
                            while(JPEG_DecodeBusy(JPEG)) __NOP();
                            
                            if(flag_firstframe == 0) //动画解析出第一帧后再将显存切换过来，否则会显示出无效图像
                            {
                                flag_firstframe = 1;
                                
                                x1 = (hres - avix->Width) / 2;
                                x2 = x1 + avix->Width - 1;
                                y1 = (vres - avix->Height) / 2;
                                y2 = y1 + avix->Height - 1;

                                //动画居中显示，不支持超过屏幕分辨率的动画
                                LCD_LayerInitStructure LCD_layerInitStruct;
                                LCD_layerInitStruct.Alpha = 0xFF;
                                LCD_layerInitStruct.HStart = x1;
                                LCD_layerInitStruct.HStop = x2;
                                LCD_layerInitStruct.VStart = y1;
                                LCD_layerInitStruct.VStop = y2;
                                LCD_layerInitStruct.DataSource = (uint32_t)splash_buf;
                                LCD_LayerInit(LCD, LCD_LAYER_1, &LCD_layerInitStruct);
                                
                                if(flag_rgb_initialized == 0)   //LCD_Start只能调用一次，用此标志位标识
                                {
                                    flag_rgb_initialized = 1;
                                    lcdc_start();
                                    bootsplash_refr(x1, x2, y1, y2, splash_buf);
                                    lcd_backlight_on();
                                } else {
                                    bootsplash_refr(x1, x2, y1, y2, splash_buf);
                                }
                            } else {
                                bootsplash_refr(x1, x2, y1, y2, splash_buf);
                            }
                            
                            t = swm_gettick() - t_start;
                            if(t < avix->SecPerFrame / 1000) //延时控制帧率
                                swm_delay_ms(avix->SecPerFrame / 1000 - t);
                            //printf("%d\r\n",swm_gettick() - t_start);
                        }
                        
                        if (avi_get_streaminfo(avix, data_buf + avix->StreamSize))
                        {
                            lv_fs_close(&file);
                            break;
                        }
                    }
                    else //音频流，暂不处理
                    {
                        lv_fs_read(&file, data_buf, avix->StreamSize + 8, NULL);

                        if (avi_get_streaminfo(avix, data_buf + avix->StreamSize))
                        {
                            lv_fs_close(&file);
                            break;
                        }
                    }
                }
                
            }
            else
            {
                printf("Boot animate is too big.\n");
            }
            lv_mem_free(avix);
        }
        else
        {
            printf("No bootsplash.\n");
        }
    }
    

    
    lv_mem_free(data_buf);
    lv_mem_free(jfif_info);
}

void bootscreen_cleanup(void)
{
    if(splash_buf) {
        lv_mem_free(splash_buf);  //切换到lvgl显存后，再释放动画显存
    }
}
