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
