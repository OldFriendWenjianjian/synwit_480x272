/**
 * @file lv_gpu_swm330_dma2d.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_gpu_swm330_dma2d.h"
#include "../lv_core/lv_disp.h"
#include "../lv_core/lv_refr.h"

#if LV_USE_GPU_SWM330_DMA2D

#include LV_GPU_SWM330_DMA2D_INCLUDE

/*********************
 *      DEFINES
 *********************/
#if LV_COLOR_DEPTH == 16
    #if LV_COLOR_16_SWAP
        #define LV_DMA2D_COLOR_FORMAT DMA2D_FMT_BGR565
    #else
        #define LV_DMA2D_COLOR_FORMAT DMA2D_FMT_RGB565
    #endif
#elif LV_COLOR_DEPTH == 24
    #define LV_DMA2D_COLOR_FORMAT DMA2D_FMT_RGB888
#elif LV_COLOR_DEPTH == 32
    #define LV_DMA2D_COLOR_FORMAT DMA2D_FMT_ARGB888
#else /*Can't use GPU with other formats*/
    #error "Can't use DMA2D with LV_COLOR_DEPTH"
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void dma2d_reset_state(void);
static void lv_draw_swm330_dma2d_blend_map(lv_color_t * dest_buf, lv_coord_t dest_w, lv_coord_t dest_h, lv_coord_t dest_stride, 
                                        const lv_color_t * src_buf, lv_coord_t src_stride, uint8_t src_color_mode, lv_opa_t opa, uint16_t *alpha_buf);
/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Turn on the peripheral and set output color mode, this only needs to be done once
 */
void lv_gpu_swm330_dma2d_init(void)
{
    DMA2D_InitStructure DMA2D_initStruct;
	DMA2D_initStruct.BurstSize = DMA2D_BURST_INC8;/* PSRAM Burst len is 32-byte, so word INC8 */
	DMA2D_initStruct.BlockSize = DMA2D_BLOCK_32;
    DMA2D_initStruct.IntEn = 0;
    DMA2D_initStruct.Interval = 1;
//    DMA2D_initStruct.Interval = CyclesPerUs * 1;
    DMA2D_Init(&DMA2D_initStruct);
}

/**
 * Fill an area in the buffer with a color
 * @param buf a buffer which should be filled
 * @param buf_w width of the buffer in pixels
 * @param color fill color
 * @param fill_w width to fill in pixels (<= buf_w)
 * @param fill_h height to fill in pixels
 * @note `buf_w - fill_w` is offset to the next line after fill
 */
void lv_gpu_swm330_dma2d_fill(lv_color_t * buf, lv_coord_t buf_w, lv_color_t color, lv_coord_t fill_w, lv_coord_t fill_h)
{
#if 1
    while (DMA2D->CR & DMA2D_CR_START_Msk) ;
    if ((uint32_t)buf >= PSRAMM_BASE) {
        while (PSRAMC->CSR & (PSRAMC_CSR_RDBUSY_Msk | PSRAMC_CSR_WRBUSY_Msk)) ;
    }
    dma2d_reset_state();

    /*DMA2D_PixelFill*/
    DMA2D->L[DMA2D_LAYER_OUT].COLOR = color.full;

    DMA2D->L[DMA2D_LAYER_OUT].MAR = (uint32_t)buf;
    DMA2D->L[DMA2D_LAYER_OUT].OR = buf_w - fill_w;
    DMA2D->L[DMA2D_LAYER_OUT].PFCCR = (LV_DMA2D_COLOR_FORMAT << DMA2D_PFCCR_CFMT_Pos);
    
    DMA2D->NLR = ((fill_h - 1) << DMA2D_NLR_NLINE_Pos) | 
                 ((fill_w - 1) << DMA2D_NLR_NPIXEL_Pos);

	DMA2D->CR &= ~(DMA2D_CR_MODE_Msk | DMA2D_CR_GPDMA_Msk);
	DMA2D->CR |= (3 << DMA2D_CR_MODE_Pos) | DMA2D_CR_START_Msk;

    while (DMA2D->CR & DMA2D_CR_START_Msk) ;
#else
    for (uint32_t y = 0; y < fill_h; y++)
    {
        for (uint32_t x = 0; x < fill_w; x++)
        {
            buf[y * buf_w + x] = color;
        }
    }
#endif
}

/**
 * Copy a map (typically RGB image) to a buffer
 * @param buf a buffer where map should be copied
 * @param buf_w width of the buffer in pixels
 * @param map an "image" to copy
 * @param map_w width of the map in pixels
 * @param copy_w width of the area to copy in pixels (<= buf_w)
 * @param copy_h height of the area to copy in pixels
 * @note `map_w - copy_w` is offset to the next line after copy
 */
void lv_gpu_swm330_dma2d_copy(lv_color_t * buf, lv_coord_t buf_w, const lv_color_t * map, lv_coord_t map_w,
                             lv_coord_t copy_w, lv_coord_t copy_h)
{
#if 1
    while (DMA2D->CR & DMA2D_CR_START_Msk) ;
    if ((uint32_t)buf >= PSRAMM_BASE || (uint32_t)map >= PSRAMM_BASE) {
        while (PSRAMC->CSR & (PSRAMC_CSR_RDBUSY_Msk | PSRAMC_CSR_WRBUSY_Msk)) ;
    }
    dma2d_reset_state();
        
    /* DMA2D_PixelMove */
	DMA2D->L[DMA2D_LAYER_FG].MAR = (uint32_t)map;
	DMA2D->L[DMA2D_LAYER_FG].OR  = map_w - copy_w;
	DMA2D->L[DMA2D_LAYER_FG].PFCCR = (LV_DMA2D_COLOR_FORMAT << DMA2D_PFCCR_CFMT_Pos);
	
	DMA2D->L[DMA2D_LAYER_OUT].MAR = (uint32_t)buf;
	DMA2D->L[DMA2D_LAYER_OUT].OR  = buf_w - copy_w;
    	
	DMA2D->NLR = ((copy_h - 1) << DMA2D_NLR_NLINE_Pos) |
				 ((copy_w - 1) << DMA2D_NLR_NPIXEL_Pos);

	DMA2D->CR &= ~(DMA2D_CR_MODE_Msk | DMA2D_CR_GPDMA_Msk);
	DMA2D->CR |= (0 << DMA2D_CR_MODE_Pos) | DMA2D_CR_START_Msk;
    
    while (DMA2D->CR & DMA2D_CR_START_Msk) ;
#else
    lv_color_t temp_buf[1024];
    for (uint32_t y = 0; y < copy_h; y++)
    {
        memcpy(temp_buf, &map[y * map_w], copy_w * sizeof(lv_color_t));
        memcpy(&buf[y * buf_w], temp_buf, copy_w * sizeof(lv_color_t));
    }
#endif
}

/**
 * Blend a map (e.g. ARGB image or RGB image with opacity) to a buffer
 * @param buf a buffer where `map` should be blended
 * @param buf_w width of the buffer in pixels
 * @param map an "image" to copy
 * @param opa opacity of `map`
 * @param map_w width of the map in pixels
 * @param blend_w width of the area to blend in pixels (<= buf_w)
 * @param blend_h height of the area to blend in pixels
 * @note `map_w - blend_w` is offset to the next line after blend
 */
void lv_gpu_swm330_dma2d_blend(lv_color_t * buf, lv_coord_t buf_w, const lv_color_t * map, lv_coord_t map_w, lv_opa_t opa, 
                              lv_coord_t blend_w, lv_coord_t blend_h)
{
    lv_draw_swm330_dma2d_blend_map(buf, blend_w, blend_h, buf_w, map, map_w, LV_DMA2D_COLOR_FORMAT, opa, NULL);
}

/**
 * Blend a map to a buffer with take into account a mask which describes the opacity of each pixel
 * @param buf a buffer where `map` should be blended
 * @param buf_w width of the buffer in pixels
 * @param map an "image" to blend
 * @param map_w width of the map in pixels
 * @param mask 0..255 values describing the opacity of the corresponding pixel. It's width is `blend_w`
 * @param blend_w width of the area to blend in pixels (<= buf_w)
 * @param blend_h height of the area to blend in pixels
 * @note `map_w - blend_w` is offset to the next line after blend
 */
void lv_gpu_swm330_dma2d_blend_mask(lv_color_t * buf, lv_coord_t buf_w, const lv_color_t * map, lv_coord_t map_w, const lv_opa_t * mask, 
                                lv_coord_t blend_w, lv_coord_t blend_h)
{
    lv_draw_swm330_dma2d_blend_map(buf, blend_w, blend_h, buf_w, map, map_w, LV_DMA2D_COLOR_FORMAT, 0xFF, (uint16_t *)mask);
}

void lv_gpu_swm330_dma2d_wait_cb(lv_disp_drv_t * drv)
{
    if(drv && drv->wait_cb) {
        while(DMA2D->CR & DMA2D_CR_START_Msk) {
            drv->wait_cb(drv);
        }
    }
    else {
        while(DMA2D->CR & DMA2D_CR_START_Msk);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void dma2d_reset_state(void)
{
	uint32_t reg_CR = DMA2D->CR;
	uint32_t reg_IE = DMA2D->IE;
	
	SYS->PRSTEN = 0x55;
	SYS->PRSTR0 |= SYS_PRSTR0_DMA2D_Msk;
    __NOP(); __NOP(); __NOP(); __NOP();
	SYS->PRSTR0 = 0;
	SYS->PRSTEN = 0;
    __NOP(); __NOP(); __NOP(); __NOP();

	DMA2D->CR = reg_CR;
	DMA2D->IE = reg_IE;
}

static void lv_draw_swm330_dma2d_blend_map(lv_color_t * dest_buf, lv_coord_t dest_w, lv_coord_t dest_h, lv_coord_t dest_stride, 
                                        const lv_color_t * src_buf, lv_coord_t src_stride, uint8_t src_color_mode, lv_opa_t opa, uint16_t *alpha_buf)
{
    while (DMA2D->CR & DMA2D_CR_START_Msk) ;
    if ((uint32_t)dest_buf >= PSRAMM_BASE || (uint32_t)src_buf >= PSRAMM_BASE) {
        while (PSRAMC->CSR & (PSRAMC_CSR_RDBUSY_Msk | PSRAMC_CSR_WRBUSY_Msk)) ;
    }
    dma2d_reset_state();

    /* DMA2D_PixelBlend: DMA2D_AMODE_PIXEL      DMA2D_AMODE_PMULA      DMA2D_AMODE_ALPHA      DMA2D_AMODE_EXTERN */
    DMA2D->L[DMA2D_LAYER_FG].MAR = (uint32_t)src_buf;
    DMA2D->L[DMA2D_LAYER_FG].OR  = src_stride - dest_w;
    DMA2D->L[DMA2D_LAYER_FG].PFCCR = (src_color_mode << DMA2D_PFCCR_CFMT_Pos) |
#if LV_COLOR_DEPTH == 32
                                     ((alpha_buf ? DMA2D_AMODE_EXTERN : DMA2D_AMODE_PMULA) << DMA2D_PFCCR_AINV_Pos) |
#else
                                     ((alpha_buf ? DMA2D_AMODE_EXTERN : DMA2D_AMODE_ALPHA) << DMA2D_PFCCR_AINV_Pos) |
#endif
                                     (opa << DMA2D_PFCCR_ALPHA_Pos);
    
    DMA2D->CR &= ~DMA2D_CR_AAREN_Msk;
    if (alpha_buf) {
        DMA2D->CR |= DMA2D_CR_AAREN_Msk;
        DMA2D->AAR = (uint32_t)alpha_buf;
    }
    
    DMA2D->L[DMA2D_LAYER_BG].MAR = (uint32_t)dest_buf;
    DMA2D->L[DMA2D_LAYER_BG].OR  = dest_stride - dest_w;
    DMA2D->L[DMA2D_LAYER_BG].PFCCR = (LV_DMA2D_COLOR_FORMAT << DMA2D_PFCCR_CFMT_Pos) | (DMA2D_AMODE_PIXEL << DMA2D_PFCCR_AINV_Pos) | (0xFF << DMA2D_PFCCR_ALPHA_Pos);
 
    DMA2D->L[DMA2D_LAYER_OUT].MAR = (uint32_t)dest_buf;
    DMA2D->L[DMA2D_LAYER_OUT].OR  = dest_stride - dest_w;
    DMA2D->L[DMA2D_LAYER_OUT].PFCCR = (LV_DMA2D_COLOR_FORMAT << DMA2D_PFCCR_CFMT_Pos);
        
    DMA2D->NLR = ((dest_h - 1) << DMA2D_NLR_NLINE_Pos) |
                ((dest_w - 1) << DMA2D_NLR_NPIXEL_Pos);
    
    DMA2D->CR &= ~(DMA2D_CR_MODE_Msk | DMA2D_CR_GPDMA_Msk);
    DMA2D->CR |= (2 << DMA2D_CR_MODE_Pos) | DMA2D_CR_START_Msk;
    
    while (DMA2D->CR & DMA2D_CR_START_Msk) ;
}

#endif
