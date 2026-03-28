/**
 * @file lv_port_disp.c
 *
 */

 /*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include "lvgl/lvgl.h"
#include "main.h"
#include "lcd/peripheral_lcd.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
#if LV_USE_GPU
static void gpu_blend(lv_disp_drv_t * disp_drv, lv_color_t * dest, const lv_color_t * src, uint32_t length, lv_opa_t opa);
static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
        const lv_area_t * fill_area, lv_color_t color);
#endif

static lv_disp_drv_t s_disp_drv;
static lv_color_t *lcdbuf_gui;
static lv_color_t *lcdbuf_show;

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(lv_coord_t hres, lv_coord_t vres, int rotation)
{
    static lv_disp_buf_t draw_buf_dsc;

    /*-------------------------
     * Initialize your display
     * -----------------------*/
    lv_disp_drv_init(&s_disp_drv);
    
    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /* LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed your display drivers `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are three buffering configurations:
     * 1. Create ONE buffer with some rows:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer with some rows:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Create TWO screen-sized buffer:
     *      Similar to 2) but the buffer have to be screen sized. When LVGL is ready it will give the
     *      whole frame to display. This way you only need to change the frame buffer's address instead of
     *      copying the pixels.
     * */
    lcdbuf_gui = (lv_color_t *)lv_mem_alloc_align(sizeof(lv_color_t) * hres * vres, 32);
    if(rotation == 90 || rotation == 180 || rotation == 270) {
        /* Display mode 1 */
        lv_color_t *draw_buf_1 = (lv_color_t *)lv_mem_alloc_align(sizeof(lv_color_t) * hres * vres, 32);
        lv_disp_buf_init(&draw_buf_dsc, draw_buf_1, NULL, hres * vres);
        
        s_disp_drv.sw_rotate = 1;
        if(rotation == 90) {
            s_disp_drv.rotated = LV_DISP_ROT_90;
        } else if(rotation == 180) {
            s_disp_drv.rotated = LV_DISP_ROT_180;
        } else {
            s_disp_drv.rotated = LV_DISP_ROT_270;
        }
    } else {
        /* Display mode 3 */
        if(lcdc_get_drive_mode() == LCDC_DRIVE_MODE_SPI_DMA || lcdc_get_drive_mode() == LCDC_DRIVE_MODE_SPI) {
            lv_disp_buf_init(&draw_buf_dsc, lcdbuf_gui, NULL,  hres * vres);   /*Initialize the display buffer*/
        }else{
            lcdbuf_show = (lv_color_t *)lv_mem_alloc_align(sizeof(lv_color_t) * hres * vres, 32);
            lv_disp_buf_init(&draw_buf_dsc, lcdbuf_gui, lcdbuf_show,  hres * vres);   /*Initialize the display buffer*/
        }
        
        s_disp_drv.sw_rotate = 0;
        s_disp_drv.rotated = LV_DISP_ROT_NONE;
    }
    
    /*Set the resolution of the display*/
    s_disp_drv.hor_res = hres;
    s_disp_drv.ver_res = vres;
    
    /*Set up the functions to access to your display*/
    s_disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    s_disp_drv.buffer = &draw_buf_dsc;

#if LV_USE_GPU
    /*Optionally add functions to access the GPU. (Only in buffered mode, LV_VDB_SIZE != 0)*/

    /*Blend two color array using opacity*/
    s_disp_drv.gpu_blend_cb = gpu_blend;

    /*Fill a memory array with a color*/
    s_disp_drv.gpu_fill_cb = gpu_fill;
#endif

    /*Finally register the driver*/
    lv_disp_drv_register(&s_disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

uint32_t timestamp;

/*Initialize your display and the required peripherals.*/
static volatile uint8_t flag_firstflush = 0;
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    if(lcdc_get_drive_mode() == LCDC_DRIVE_MODE_MPU) {
        /* MPU模式 */
        MPU_LCD.setcur(area->x1, area->x2, area->y1, area->y2);//设置输出像素的显示区域	
        for(lv_coord_t y = area->y1; y <= area->y2; y++) {
            for(lv_coord_t x = area->x1; x <= area->x2; x++) {
                LCD_WR_DATA(LCD, color_p->full);
                color_p++;// 16 bit
            }
        }
    } else if(lcdc_get_drive_mode() == LCDC_DRIVE_MODE_SPI) {
        /* SPI模式 */
        extern void lcd_write_data_spi_4line(uint16_t data);
        
        MPU_LCD.setcur(area->x1, area->x2, area->y1, area->y2);
		for(lv_coord_t y = area->y1; y <= area->y2; y++) {
			for(lv_coord_t x = area->x1; x <= area->x2; x++) {
                lcd_write_data_spi_4line( color_p->full >> 8);
                lcd_write_data_spi_4line( color_p->full &0xFF );			
				color_p++;// 16 bit
			}
		}
    }
#if defined(SWM34SCE_A1)
    else if(lcdc_get_drive_mode() == LCDC_DRIVE_MODE_SPI_DMA) {
        /* SPI DMA模式 */
        MPU_LCD.setcur(area->x1, area->x2, area->y1, area->y2);
#if 1
        uint32_t cnt = (area->y2 - area->y1 + 1) * (area->x2 - area->x1 + 1);
    
        extern uint8_t flush_bitmap_async_spi_8(void *data, uint32_t pixels);
        uint8_t res = flush_bitmap_async_spi_8(color_p, cnt);
        if (res) while (1) __NOP();
#endif
    } 
#endif
    else {
        /* RGB模式 */
        if(s_disp_drv.sw_rotate && s_disp_drv.rotated != LV_DISP_ROT_NONE) {
            lv_color_t *line_start = &lcdbuf_gui[area->y1 * s_disp_drv.hor_res + area->x1];
            uint32_t i;
            for(lv_coord_t y = area->y1; y <= area->y2; y++) {
                i = 0;
                for(lv_coord_t x = area->x1; x <= area->x2; x++) {
                    line_start[i] = *color_p;
                    i++;
                    color_p++;
                }
                line_start += s_disp_drv.hor_res;
            }
        } else {
            if(flag_firstflush == 0) //第一次准备素材时不切换显存，否则会显示出刷新过程
            {
                flag_firstflush = 1;
            }
            else
            {
                if(!flag_rgb_initialized) {
                    lv_disp_flush_ready(disp_drv);
                    return;
                }
                        
                LCD->L[0].ADDR = (uint32_t)color_p;
                LCD->CR |= (1 << LCD_CR_VBPRELOAD_Pos);
                while(LCD->CR & LCD_CR_VBPRELOAD_Msk);
            }
        }
    }
    
    if(lcdc_get_drive_mode() != LCDC_DRIVE_MODE_SPI_DMA) {
        /*IMPORTANT!!!
         *Inform the graphics library that you are ready with the flushing*/
        lv_disp_flush_ready(disp_drv);
    }
}

#if defined(SWM34SCE_A1)
void DMA_Handler(void)
{
#if 1
		extern uint8_t flush_bitmap_async_done_spi_8(void);
		if (0 == flush_bitmap_async_done_spi_8())
		{
            lv_disp_flush_ready(&s_disp_drv);
		}
#endif
}
#endif

void lvgl_layer_init(void)
{
    //素材准备好后再切换显存
    LCD_LayerInitStructure LCD_layerInitStruct;
    LCD_layerInitStruct.Alpha = 0xFF;
    LCD_layerInitStruct.HStart = 0;
    LCD_layerInitStruct.HStop = s_disp_drv.hor_res - 1;
    LCD_layerInitStruct.VStart = 0;
    LCD_layerInitStruct.VStop = s_disp_drv.ver_res - 1;
    LCD_layerInitStruct.DataSource = (uint32_t)lcdbuf_gui;
    LCD_LayerInit(LCD, LCD_LAYER_1, &LCD_layerInitStruct);
    if( flag_rgb_initialized == 0)  //LCD_Start只能调用一次，用此标志位标识
    {
        flag_rgb_initialized = 1;
        lcdc_start();
        swm_delay_ms(100);
        lcd_backlight_on();
    }
}
/*OPTIONAL: GPU INTERFACE*/
#if LV_USE_GPU

/* If your MCU has hardware accelerator (GPU) then you can use it to blend to memories using opacity
 * It can be used only in buffered mode (LV_VDB_SIZE != 0 in lv_conf.h)*/
static void gpu_blend(lv_disp_drv_t * disp_drv, lv_color_t * dest, const lv_color_t * src, uint32_t length, lv_opa_t opa)
{
    /*It's an example code which should be done by your GPU*/
    uint32_t i;
    for(i = 0; i < length; i++) {
        dest[i] = lv_color_mix(dest[i], src[i], opa);
    }
}

/* If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color
 * It can be used only in buffered mode (LV_VDB_SIZE != 0 in lv_conf.h)*/
static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
                    const lv_area_t * fill_area, lv_color_t color)
{
    /*It's an example code which should be done by your GPU*/
    int32_t x, y;
    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/

    for(y = fill_area->y1; y <= fill_area->y2; y++) {
        for(x = fill_area->x1; x <= fill_area->x2; x++) {
            dest_buf[x] = color;
        }
        dest_buf+=dest_width;    /*Go to the next line*/
    }
}

#endif  /*LV_USE_GPU*/

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
