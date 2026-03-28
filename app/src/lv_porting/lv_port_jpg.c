
/*********************
 *      INCLUDES
 *********************/

#include "lvgl/lvgl.h"
#include "lv_port_jpg.h"
#include "board.h"
#include "jfif_parser.h"

/*********************
 *      DEFINES
 *********************/
#define TJPGD_WORKBUFF_SIZE             4096    //Recommended by TJPGD libray

//NEVER EDIT THESE OFFSET VALUES
#define SJPEG_VERSION_OFFSET            8
#define SJPEG_X_RES_OFFSET              14
#define SJPEG_y_RES_OFFSET              16
#define SJPEG_TOTAL_FRAMES_OFFSET       18
#define SJPEG_BLOCK_WIDTH_OFFSET        20
#define SJPEG_FRAME_INFO_ARRAY_OFFSET   22

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    uint8_t * sjpeg_data;
    uint32_t sjpeg_data_size;
    int sjpeg_x_res;
    int sjpeg_y_res;
    int sjpeg_total_frames;
    int sjpeg_single_frame_height;
    int sjpeg_cache_frame_index;
    uint8_t ** frame_base_array;        //to save base address of each split frames upto sjpeg_total_frames.
    int * frame_base_offset;            //to save base offset for fseek
    uint8_t * frame_cache;
    jfif_info_t * jfif_info;
    int frame_stride;
} SJPEG;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_res_t decoder_info(lv_img_decoder_t * decoder, const void * src, lv_img_header_t * header);
static lv_res_t decoder_open(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc);
static lv_res_t decoder_read_line(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc, lv_coord_t x, lv_coord_t y,
                                  lv_coord_t len, uint8_t * buf);
static lv_res_t decoder_uncompress(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc, lv_color_t* external_output_buffer, uint32_t out_buffer_stride);
static void decoder_close(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc);
static int is_jpg(const uint8_t * raw_data, size_t len);
static void lv_sjpg_cleanup(SJPEG * sjpeg);
static void lv_sjpg_free(SJPEG * sjpeg);

/**********************
 *  STATIC VARIABLES
 **********************/
static void black8x8();

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_port_jpeg_init(void)
{
    lv_img_decoder_t * dec = lv_img_decoder_create();
    lv_img_decoder_set_info_cb(dec, decoder_info);
    lv_img_decoder_set_open_cb(dec, decoder_open);
    lv_img_decoder_set_close_cb(dec, decoder_close);
    lv_img_decoder_set_read_line_cb(dec, decoder_read_line);
    lv_img_decoder_set_uncompress_cb(dec, decoder_uncompress);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
/**
 * Get info about an SJPG / JPG image
 * @param decoder pointer to the decoder where this function belongs
 * @param src can be file name or pointer to a C array
 * @param header store the info here
 * @return LV_RES_OK: no error; LV_RES_INV: can't get the info
 */
static lv_res_t decoder_info(lv_img_decoder_t * decoder, const void * src, lv_img_header_t * header)
{
    LV_UNUSED(decoder);

    /*Check whether the type `src` is known by the decoder*/
    /* Read the SJPG/JPG header and find `width` and `height` */

    lv_img_src_t src_type = lv_img_src_get_type(src);          /*Get the source type*/

    lv_res_t ret = LV_RES_OK;

    if(src_type == LV_IMG_SRC_VARIABLE) {
        const lv_img_dsc_t * img_dsc = src;
        uint8_t * raw_sjpeg_data = (uint8_t *)img_dsc->data;
        const uint32_t raw_sjpeg_data_size = img_dsc->data_size;

        if (is_jpg(raw_sjpeg_data, raw_sjpeg_data_size) == true) {
            header->always_zero = 0;
            header->cf = LV_IMG_CF_RAW;
            jfif_info_t* jfif_info = lv_mem_alloc(sizeof(jfif_info_t));
            if (!jfif_info) {
                return LV_RES_INV;
            }
            int res = jfif_parse(raw_sjpeg_data, raw_sjpeg_data_size, jfif_info);
            if (res == JFIF_RES_OK) {
                header->w = jfif_info->Width;
                header->h = jfif_info->Height;
            }
            else {
                ret = LV_RES_INV;
            }
            lv_mem_free(jfif_info);
            return ret;
        }
    }
    else if(src_type == LV_IMG_SRC_FILE) {
        const char * fn = src;
        if(strcmp(lv_fs_get_ext(fn), "jpg") == 0) {
            lv_fs_file_t file;
            lv_fs_res_t res = lv_fs_open(&file, fn, LV_FS_MODE_RD);
            if(res != LV_FS_RES_OK) return 78;

            uint8_t * workb_temp = lv_mem_alloc(TJPGD_WORKBUFF_SIZE);
            if(!workb_temp) {
                lv_fs_close(&file);
                return LV_RES_INV;
            }
        }
    }
    return LV_RES_INV;
}

/**
 * Open SJPG image and return the decided image
 * @param decoder pointer to the decoder where this function belongs
 * @param dsc pointer to a descriptor which describes this decoding session
 * @return LV_RES_OK: no error; LV_RES_INV: can't get the info
 */
static lv_res_t decoder_open(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder);
    lv_res_t lv_ret = LV_RES_OK;

    if(dsc->src_type == LV_IMG_SRC_VARIABLE) {
        uint8_t * data;
        SJPEG * sjpeg = (SJPEG *) dsc->user_data;
        const uint32_t raw_sjpeg_data_size = ((lv_img_dsc_t *)dsc->src)->data_size;
        if(sjpeg == NULL) {
            sjpeg =  lv_mem_alloc(sizeof(SJPEG));
            if(!sjpeg) return LV_RES_INV;

            memset(sjpeg, 0, sizeof(SJPEG));

            dsc->user_data = sjpeg;
            sjpeg->sjpeg_data = (uint8_t *)((lv_img_dsc_t *)(dsc->src))->data;
            sjpeg->sjpeg_data_size = ((lv_img_dsc_t *)(dsc->src))->data_size;
        }

        if(is_jpg(sjpeg->sjpeg_data, raw_sjpeg_data_size) == true) {

            sjpeg->jfif_info = lv_mem_alloc(sizeof(jfif_info_t));
            if (!sjpeg->jfif_info) {
                lv_sjpg_cleanup(sjpeg);
                sjpeg = NULL;
                dsc->user_data = NULL;
                return LV_RES_INV;
            }
            int res = jfif_parse(sjpeg->sjpeg_data, sjpeg->sjpeg_data_size, sjpeg->jfif_info);
            if (res == JFIF_RES_OK) {
                sjpeg->sjpeg_x_res = sjpeg->jfif_info->Width;
                sjpeg->sjpeg_y_res = sjpeg->jfif_info->Height;
                sjpeg->sjpeg_total_frames = 1;
                sjpeg->sjpeg_single_frame_height = sjpeg->jfif_info->Height;

                sjpeg->frame_base_array = lv_mem_alloc(sizeof(uint8_t*) * sjpeg->sjpeg_total_frames);
                if (!sjpeg->frame_base_array) {
                    lv_sjpg_cleanup(sjpeg);
                    sjpeg = NULL;
                    dsc->user_data = NULL;
                    return LV_RES_INV;
                }
                sjpeg->frame_base_offset = NULL;

                uint8_t* img_frame_base = sjpeg->sjpeg_data;
                sjpeg->frame_base_array[0] = img_frame_base;
                sjpeg->sjpeg_cache_frame_index = -1;
            }
            else {
                lv_sjpg_cleanup(sjpeg);
                sjpeg = NULL;
                dsc->user_data = NULL;
                lv_ret = LV_RES_INV;
            }
            return lv_ret;
        }
    }

    return LV_RES_INV;
}

/**
 * Decode `len` pixels starting from the given `x`, `y` coordinates and store them in `buf`.
 * Required only if the "open" function can't open the whole decoded pixel array. (dsc->img_data == NULL)
 * @param decoder pointer to the decoder the function associated with
 * @param dsc pointer to decoder descriptor
 * @param x start x coordinate
 * @param y start y coordinate
 * @param len number of pixels to decode
 * @param buf a buffer to store the decoded pixels
 * @return LV_RES_OK: ok; LV_RES_INV: failed
 */

static lv_res_t decoder_read_line(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc, lv_coord_t x, lv_coord_t y,
                                  lv_coord_t len, uint8_t * buf)
{
    LV_UNUSED(decoder);
    if(dsc->src_type == LV_IMG_SRC_VARIABLE) {
        SJPEG * sjpeg = (SJPEG *) dsc->user_data;
        int sjpeg_req_frame_index = y / sjpeg->sjpeg_single_frame_height;

        /*If line not from cache, refresh cache */
        if (sjpeg_req_frame_index != sjpeg->sjpeg_cache_frame_index) {
            jpeg_outset_t jpeg_outset;
            if (dsc->mode == LV_IMG_DECODE_MODE_Y) {
                jpeg_outset.format = JPEG_OUT_YUV;
                sjpeg->frame_stride = (int)ceil(sjpeg->jfif_info->Width / 4.0) * 4;
                sjpeg->frame_cache = (void*)lv_mem_alloc(sjpeg->frame_stride * (sjpeg->jfif_info->Height + 1));
                if (!sjpeg->frame_cache) {
                    return LV_RES_INV;
                }

                jpeg_outset.dither = 0;
                jpeg_outset.YAddr = (uint32_t)sjpeg->frame_cache;
                jpeg_outset.CbAddr = (uint32_t)&(sjpeg->frame_cache[sjpeg->frame_stride * sjpeg->jfif_info->Height]);
                jpeg_outset.CrAddr = (uint32_t)&(sjpeg->frame_cache[sjpeg->frame_stride * sjpeg->jfif_info->Height]);
            }
            else {

#if  LV_COLOR_DEPTH == 32
                jpeg_outset.format = JPEG_OUT_XRGB888;
                sjpeg->frame_stride = sjpeg->jfif_info->Width * 4;
                sjpeg->frame_cache = (void*)lv_mem_alloc(sjpeg->frame_stride * sjpeg->jfif_info->Height);
#elif  LV_COLOR_DEPTH == 16
                jpeg_outset.format = JPEG_OUT_RGB565;
                sjpeg->frame_stride = (int)ceil(sjpeg->jfif_info->Width / 2.0) * 4;
                sjpeg->frame_cache = (void*)lv_mem_alloc(sjpeg->frame_stride * sjpeg->jfif_info->Height);
#endif
                if (!sjpeg->frame_cache) {
                    return LV_RES_INV;
                }

                jpeg_outset.dither = 0;
                jpeg_outset.RGBAddr = (uint32_t)sjpeg->frame_cache;
            }

            uint32_t t_start = 0,retry_cnt = 0;
JPEG_RETRY:
            t_start = lv_tick_get();
            if(retry_cnt > 0)
            {
                JPEG->CR |= (1 << JPEG_CR_RESET_Pos);
                while(JPEG->CR & JPEG_CR_RESET_Msk);
            }
            jpeg_decode(JPEG, sjpeg->jfif_info, &jpeg_outset, sjpeg->jfif_info->Width);
            while (JPEG_DecodeBusy(JPEG))
            {
                if(lv_tick_get() - t_start > 30)
                {
                    retry_cnt++;
                    //printf("retry:%d\n",retry_cnt);
                    if(retry_cnt > 3) //出错重试三次
                    {
                        lv_mem_free(sjpeg->frame_cache);
                        sjpeg->frame_cache = NULL;
                        return LV_RES_INV;
                    }
                    goto JPEG_RETRY;
                }
                __NOP();
            }
            
            if((JPEG->SR & JPEG_SR_CUOVR_Msk) || (JPEG->SR & JPEG_SR_REIMERR_Msk)) {
                //printf("SR error retry:%d,width:%d,height:%d\n",retry_cnt,sjpeg->jfif_info->Width,sjpeg->jfif_info->Height);
                retry_cnt++;
                if(retry_cnt > 3) //出错重试三次
                {
                    lv_mem_free(sjpeg->frame_cache);
                    sjpeg->frame_cache = NULL;
                    return LV_RES_INV;
                }
                jfif_parse(sjpeg->sjpeg_data, sjpeg->sjpeg_data_size, sjpeg->jfif_info);
                goto JPEG_RETRY;
            }
            
            sjpeg->sjpeg_cache_frame_index = sjpeg_req_frame_index;
        }
        
        if (dsc->mode == LV_IMG_DECODE_MODE_Y) {
            uint8_t* cache = (uint8_t*)sjpeg->frame_cache + x + y * sjpeg->frame_stride;
            memcpy(buf, cache, len);
        }
        else {
#if  LV_COLOR_DEPTH == 32
        uint8_t* cache = (uint8_t*)sjpeg->frame_cache + x * 4 + y * sjpeg->frame_stride;
        memcpy(buf, cache, len * 4);
#elif  LV_COLOR_DEPTH == 16
        uint8_t* cache = (uint8_t*)sjpeg->frame_cache + x * 2 + y * sjpeg->frame_stride;
        memcpy(buf, cache, len * 2);
#endif
        }

        return LV_RES_OK;
    }
    else if(dsc->src_type == LV_IMG_SRC_FILE) {
        SJPEG * sjpeg = (SJPEG *) dsc->user_data;

        int offset = 0;
        uint8_t * cache = (uint8_t *)sjpeg->frame_cache + x * 3 + (y % sjpeg->sjpeg_single_frame_height) * sjpeg->sjpeg_x_res *
                          3;

        uint8_t r, g, b;
        for (int i = 0; i < len; i++) {
            r = *cache++;
            g = *cache++;
            b = *cache++;

            if (dsc->mode == LV_IMG_DECODE_MODE_Y) {
                buf[i] = (uint8_t)(0.3 * r + 0.59 * g + 0.11 * b);
            }
            else {
#if LV_COLOR_DEPTH == 32
                buf[offset + 3] = 0xff;
                buf[offset + 2] = r;
                buf[offset + 1] = g;
                buf[offset + 0] = b;
                offset += 4;
            }
#elif  LV_COLOR_DEPTH == 16
                uint16_t col_8bit = (r & 0xf8) << 8;
                col_8bit |= (g & 0xFC) << 3;
                col_8bit |= (b >> 3);
#if  LV_BIG_ENDIAN_SYSTEM == 1 || LV_COLOR_16_SWAP == 1
                buf[offset++] = col_8bit >> 8;
                buf[offset++] = col_8bit & 0xff;
#else
                buf[offset++] = col_8bit & 0xff;
                buf[offset++] = col_8bit >> 8;
#endif // LV_BIG_ENDIAN_SYSTEM
            }

#elif  LV_COLOR_DEPTH == 8
                uint8_t col_8bit = (r & 0xC0);
                col_8bit |= (g & 0xe0) >> 2;
                col_8bit |= (b & 0xe0) >> 5;
                buf[offset++] = col_8bit;
        }

#else
#error Unsupported LV_COLOR_DEPTH
#endif // LV_COLOR_DEPTH
        }
        return LV_RES_OK;
    }
end:
    return LV_RES_INV;
}

static lv_res_t decoder_uncompress(lv_img_decoder_t * decoder, 
    lv_img_decoder_dsc_t * dsc, 
    lv_color_t* optional_output_buffer, uint32_t out_buffer_stride)
{
    SJPEG * sjpeg = (SJPEG *) dsc->user_data;
    jpeg_outset_t jpeg_outset;
    uint32_t stride;
    if (dsc->mode == LV_IMG_DECODE_MODE_Y) {
        jpeg_outset.format = JPEG_OUT_YUV;
        jpeg_outset.dither = 0;

        sjpeg->frame_stride = (int)ceil(sjpeg->jfif_info->Width / 4.0) * 4;
        sjpeg->frame_cache = (void*)lv_mem_alloc(sjpeg->frame_stride * (sjpeg->jfif_info->Height + 1));
        if (!sjpeg->frame_cache) {
            return LV_RES_INV;
        }

        jpeg_outset.YAddr = (uint32_t)sjpeg->frame_cache;
        jpeg_outset.CbAddr = (uint32_t)&(sjpeg->frame_cache[sjpeg->frame_stride * sjpeg->jfif_info->Height]);
        jpeg_outset.CrAddr = (uint32_t)&(sjpeg->frame_cache[sjpeg->frame_stride * sjpeg->jfif_info->Height]);
        stride = sjpeg->jfif_info->Width;
    }
    else {
        jpeg_outset.dither = 0;
#if  LV_COLOR_DEPTH == 32
        jpeg_outset.format = JPEG_OUT_XRGB888;
#elif  LV_COLOR_DEPTH == 16
        jpeg_outset.format = JPEG_OUT_RGB565;
#endif
        
        if(optional_output_buffer == NULL || out_buffer_stride == 0) {
            if(jpeg_outset.format == JPEG_OUT_XRGB888) {
                sjpeg->frame_stride = sjpeg->jfif_info->Width * 4;
                sjpeg->frame_cache = (void*)lv_mem_alloc(sjpeg->frame_stride * sjpeg->jfif_info->Height);
            } else {
                sjpeg->frame_stride = (int)ceil(sjpeg->jfif_info->Width / 2.0) * 4;
                sjpeg->frame_cache = (void*)lv_mem_alloc(sjpeg->frame_stride * sjpeg->jfif_info->Height);
            }
            if (!sjpeg->frame_cache) {
                return LV_RES_INV;
            }
            jpeg_outset.RGBAddr = (uint32_t)sjpeg->frame_cache;
            stride = sjpeg->jfif_info->Width;
        } else {
            jpeg_outset.RGBAddr = (uint32_t)optional_output_buffer;
            stride = out_buffer_stride / sizeof(lv_color_t);
        }
    }
    
    uint32_t t_start = 0,retry_cnt = 0;
JPEG_RETRY:
    t_start = lv_tick_get();
    if(retry_cnt > 0)
    {
        JPEG->CR |= (1 << JPEG_CR_RESET_Pos);
        while(JPEG->CR & JPEG_CR_RESET_Msk);
    }
    jpeg_decode(JPEG, sjpeg->jfif_info, &jpeg_outset, stride);
    while (JPEG_DecodeBusy(JPEG))
    {
        if(lv_tick_get() - t_start > 30)
        {
            retry_cnt++;
            //printf("retry:%d\n",retry_cnt);
            if(retry_cnt > 3) //出错重试三次
            {
                lv_mem_free(sjpeg->frame_cache);
                sjpeg->frame_cache = NULL;
                return LV_RES_INV;
            }
            goto JPEG_RETRY;
        }
        __NOP();
    }
    
    if((JPEG->SR & JPEG_SR_CUOVR_Msk) || (JPEG->SR & JPEG_SR_REIMERR_Msk)) {
        //printf("SR error retry:%d,width:%d,height:%d\n",retry_cnt,sjpeg->jfif_info->Width,sjpeg->jfif_info->Height);
        retry_cnt++;
        if(retry_cnt > 3) //出错重试三次
        {
            lv_mem_free(sjpeg->frame_cache);
            sjpeg->frame_cache = NULL;
            return LV_RES_INV;
        }
        jfif_parse(sjpeg->sjpeg_data, sjpeg->sjpeg_data_size, sjpeg->jfif_info);
        goto JPEG_RETRY;
    }
    
    if(optional_output_buffer == NULL || stride == 0 ||
        dsc->mode == LV_IMG_DECODE_MODE_Y) {

        dsc->img_data = (uint8_t *)sjpeg->frame_cache;
        dsc->stride = sjpeg->frame_stride;
            
        sjpeg->sjpeg_cache_frame_index = 0;
    } else {
        dsc->img_data = (uint8_t *)optional_output_buffer;
        dsc->stride = out_buffer_stride;
    }
    
    //black8x8();
    return LV_RES_OK;
}

/**
 * Free the allocated resources
 * @param decoder pointer to the decoder where this function belongs
 * @param dsc pointer to a descriptor which describes this decoding session
 */
static void decoder_close(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder);
    /*Free all allocated data*/
    SJPEG * sjpeg = (SJPEG *) dsc->user_data;
    if(!sjpeg) return;

    switch(dsc->src_type) {
        case LV_IMG_SRC_FILE:
            lv_sjpg_cleanup(sjpeg);
            break;

        case LV_IMG_SRC_VARIABLE:
            lv_sjpg_cleanup(sjpeg);
            break;

        default:
            ;
    }
}

static int is_jpg(const uint8_t * raw_data, size_t len)
{
    const uint8_t jpg_signature[] = {0xFF, 0xD8};
    if(len < sizeof(jpg_signature)) return false;
    return memcmp(jpg_signature, raw_data, sizeof(jpg_signature)) == 0;
}

static void lv_sjpg_free(SJPEG * sjpeg)
{
    if(sjpeg->frame_cache) lv_mem_free(sjpeg->frame_cache);
    if(sjpeg->frame_base_array) lv_mem_free(sjpeg->frame_base_array);
    if(sjpeg->frame_base_offset) lv_mem_free(sjpeg->frame_base_offset);
    if (sjpeg->jfif_info) lv_mem_free(sjpeg->jfif_info);
}

static void lv_sjpg_cleanup(SJPEG * sjpeg)
{
    if(! sjpeg) return;

    lv_sjpg_free(sjpeg);
    lv_mem_free(sjpeg);
}

static void black8x8()
{
    static const uint8_t jpeg_black8x8[1381] = {
	0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
	0x01, 0x01, 0x01, 0x2C, 0x01, 0x2C, 0x00, 0x00, 0xFF, 0xE1, 0x00, 0x22,
	0x45, 0x78, 0x69, 0x66, 0x00, 0x00, 0x4D, 0x4D, 0x00, 0x2A, 0x00, 0x00,
	0x00, 0x08, 0x00, 0x01, 0x01, 0x12, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x14,
	0x43, 0x72, 0x65, 0x61, 0x74, 0x65, 0x64, 0x20, 0x77, 0x69, 0x74, 0x68,
	0x20, 0x47, 0x49, 0x4D, 0x50, 0x00, 0xFF, 0xE2, 0x02, 0xB0, 0x49, 0x43,
	0x43, 0x5F, 0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45, 0x00, 0x01, 0x01,
	0x00, 0x00, 0x02, 0xA0, 0x6C, 0x63, 0x6D, 0x73, 0x04, 0x30, 0x00, 0x00,
	0x6D, 0x6E, 0x74, 0x72, 0x52, 0x47, 0x42, 0x20, 0x58, 0x59, 0x5A, 0x20,
	0x07, 0xE9, 0x00, 0x0B, 0x00, 0x15, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x1C,
	0x61, 0x63, 0x73, 0x70, 0x4D, 0x53, 0x46, 0x54, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF6, 0xD6,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xD3, 0x2D, 0x6C, 0x63, 0x6D, 0x73,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D,
	0x64, 0x65, 0x73, 0x63, 0x00, 0x00, 0x01, 0x20, 0x00, 0x00, 0x00, 0x40,
	0x63, 0x70, 0x72, 0x74, 0x00, 0x00, 0x01, 0x60, 0x00, 0x00, 0x00, 0x36,
	0x77, 0x74, 0x70, 0x74, 0x00, 0x00, 0x01, 0x98, 0x00, 0x00, 0x00, 0x14,
	0x63, 0x68, 0x61, 0x64, 0x00, 0x00, 0x01, 0xAC, 0x00, 0x00, 0x00, 0x2C,
	0x72, 0x58, 0x59, 0x5A, 0x00, 0x00, 0x01, 0xD8, 0x00, 0x00, 0x00, 0x14,
	0x62, 0x58, 0x59, 0x5A, 0x00, 0x00, 0x01, 0xEC, 0x00, 0x00, 0x00, 0x14,
	0x67, 0x58, 0x59, 0x5A, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x14,
	0x72, 0x54, 0x52, 0x43, 0x00, 0x00, 0x02, 0x14, 0x00, 0x00, 0x00, 0x20,
	0x67, 0x54, 0x52, 0x43, 0x00, 0x00, 0x02, 0x14, 0x00, 0x00, 0x00, 0x20,
	0x62, 0x54, 0x52, 0x43, 0x00, 0x00, 0x02, 0x14, 0x00, 0x00, 0x00, 0x20,
	0x63, 0x68, 0x72, 0x6D, 0x00, 0x00, 0x02, 0x34, 0x00, 0x00, 0x00, 0x24,
	0x64, 0x6D, 0x6E, 0x64, 0x00, 0x00, 0x02, 0x58, 0x00, 0x00, 0x00, 0x24,
	0x64, 0x6D, 0x64, 0x64, 0x00, 0x00, 0x02, 0x7C, 0x00, 0x00, 0x00, 0x24,
	0x6D, 0x6C, 0x75, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x0C, 0x65, 0x6E, 0x55, 0x53, 0x00, 0x00, 0x00, 0x24,
	0x00, 0x00, 0x00, 0x1C, 0x00, 0x47, 0x00, 0x49, 0x00, 0x4D, 0x00, 0x50,
	0x00, 0x20, 0x00, 0x62, 0x00, 0x75, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x74,
	0x00, 0x2D, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x20, 0x00, 0x73, 0x00, 0x52,
	0x00, 0x47, 0x00, 0x42, 0x6D, 0x6C, 0x75, 0x63, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0C, 0x65, 0x6E, 0x55, 0x53,
	0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x50, 0x00, 0x75,
	0x00, 0x62, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x63, 0x00, 0x20, 0x00, 0x44,
	0x00, 0x6F, 0x00, 0x6D, 0x00, 0x61, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x00,
	0x58, 0x59, 0x5A, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF6, 0xD6,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xD3, 0x2D, 0x73, 0x66, 0x33, 0x32,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0C, 0x42, 0x00, 0x00, 0x05, 0xDE,
	0xFF, 0xFF, 0xF3, 0x25, 0x00, 0x00, 0x07, 0x93, 0x00, 0x00, 0xFD, 0x90,
	0xFF, 0xFF, 0xFB, 0xA1, 0xFF, 0xFF, 0xFD, 0xA2, 0x00, 0x00, 0x03, 0xDC,
	0x00, 0x00, 0xC0, 0x6E, 0x58, 0x59, 0x5A, 0x20, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x6F, 0xA0, 0x00, 0x00, 0x38, 0xF5, 0x00, 0x00, 0x03, 0x90,
	0x58, 0x59, 0x5A, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x9F,
	0x00, 0x00, 0x0F, 0x84, 0x00, 0x00, 0xB6, 0xC4, 0x58, 0x59, 0x5A, 0x20,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x97, 0x00, 0x00, 0xB7, 0x87,
	0x00, 0x00, 0x18, 0xD9, 0x70, 0x61, 0x72, 0x61, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x66, 0x66, 0x00, 0x00, 0xF2, 0xA7,
	0x00, 0x00, 0x0D, 0x59, 0x00, 0x00, 0x13, 0xD0, 0x00, 0x00, 0x0A, 0x5B,
	0x63, 0x68, 0x72, 0x6D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
	0x00, 0x00, 0xA3, 0xD7, 0x00, 0x00, 0x54, 0x7C, 0x00, 0x00, 0x4C, 0xCD,
	0x00, 0x00, 0x99, 0x9A, 0x00, 0x00, 0x26, 0x67, 0x00, 0x00, 0x0F, 0x5C,
	0x6D, 0x6C, 0x75, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x0C, 0x65, 0x6E, 0x55, 0x53, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x1C, 0x00, 0x47, 0x00, 0x49, 0x00, 0x4D, 0x00, 0x50,
	0x6D, 0x6C, 0x75, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x0C, 0x65, 0x6E, 0x55, 0x53, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x1C, 0x00, 0x73, 0x00, 0x52, 0x00, 0x47, 0x00, 0x42,
	0xFF, 0xDB, 0x00, 0x43, 0x00, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x05, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x06, 0x04, 0x04, 0x03, 0x05, 0x07, 0x06, 0x07, 0x07, 0x07,
	0x06, 0x07, 0x07, 0x08, 0x09, 0x0B, 0x09, 0x08, 0x08, 0x0A, 0x08, 0x07,
	0x07, 0x0A, 0x0D, 0x0A, 0x0A, 0x0B, 0x0C, 0x0C, 0x0C, 0x0C, 0x07, 0x09,
	0x0E, 0x0F, 0x0D, 0x0C, 0x0E, 0x0B, 0x0C, 0x0C, 0x0C, 0xFF, 0xDB, 0x00,
	0x43, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x06, 0x03, 0x03, 0x06,
	0x0C, 0x08, 0x07, 0x08, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00,
	0x08, 0x00, 0x08, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11,
	0x01, 0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xC4,
	0x00, 0xB5, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05,
	0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04,
	0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22,
	0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15,
	0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36,
	0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
	0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66,
	0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
	0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95,
	0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
	0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2,
	0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5,
	0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9,
	0xFA, 0xFF, 0xC4, 0x00, 0x1F, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xC4,
	0x00, 0xB5, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07,
	0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11,
	0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13,
	0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23,
	0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1,
	0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35,
	0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65,
	0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93,
	0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
	0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9,
	0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3,
	0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
	0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9,
	0xFA, 0xFF, 0xDA, 0x00, 0x0C, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11,
	0x00, 0x3F, 0x00, 0xFC, 0x02, 0xF9, 0xBD, 0xA8, 0xA2, 0x8A, 0x00, 0xFF,
	0xD9
    };
    static jfif_info_t* black8x8_jfif_info = NULL;
    uint8_t black8x8_frame[8 * 8 + 8];
    
    if(black8x8_jfif_info == NULL) {
        black8x8_jfif_info = lv_mem_alloc(sizeof(jfif_info_t));
        if(black8x8_jfif_info == NULL) {
            return;
        }
        jfif_parse(jpeg_black8x8, sizeof(jpeg_black8x8), black8x8_jfif_info);
    }

    jpeg_outset_t jpeg_outset;
    jpeg_outset.format = JPEG_OUT_YUV;
    jpeg_outset.dither = 0;
    jpeg_outset.YAddr = (uint32_t)black8x8_frame;
    jpeg_outset.CbAddr = (uint32_t)&(black8x8_frame[8*8]);
    jpeg_outset.CrAddr = (uint32_t)&(black8x8_frame[8*8]);  
    jpeg_decode(JPEG, black8x8_jfif_info, &jpeg_outset, black8x8_jfif_info->Width);
    
    uint32_t t_start = lv_tick_get();
    while (JPEG_DecodeBusy(JPEG)) {
        if(lv_tick_get() - t_start > 30) {
            break;
        }
        __NOP();
    }
}