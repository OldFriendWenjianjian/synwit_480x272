/**
 * @file lv_draw_blend.h
 *
 */

#ifndef LV_DRAW_BLEND_H
#define LV_DRAW_BLEND_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_misc/lv_color.h"
#include "../lv_misc/lv_area.h"
#include "lv_draw_mask.h"
#include "../lv_hal/lv_hal_disp.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
enum {
    LV_BLEND_MODE_NORMAL,
#if LV_USE_BLEND_MODES
    LV_BLEND_MODE_ADDITIVE,
    LV_BLEND_MODE_SUBTRACTIVE,
#endif
};

typedef uint8_t lv_blend_mode_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

//! @cond Doxygen_Suppress
LV_EXPORT_API LV_ATTRIBUTE_FAST_MEM void _lv_blend_fill(const lv_area_t * clip_area, const lv_area_t * fill_area, lv_color_t color,
                                          lv_opa_t * mask, lv_draw_mask_res_t mask_res, lv_opa_t opa, lv_blend_mode_t mode);

LV_EXPORT_API LV_ATTRIBUTE_FAST_MEM void _lv_blend_map(const lv_area_t * clip_area, const lv_area_t * map_area,
                                         const lv_color_t * map_buf,
                                         lv_opa_t * mask, lv_draw_mask_res_t mask_res, lv_opa_t opa, lv_blend_mode_t mode);
LV_EXPORT_API lv_color_t* _lv_disp_buf_projection_first(lv_disp_t* disp, lv_color_t* disp_buf,
                                        const lv_area_t* disp_area, const lv_area_t* draw_area,
                                        int32_t* disp_width);
//! @endcond
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DRAW_BLEND_H*/
