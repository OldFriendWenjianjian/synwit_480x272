/**
 * @file lv_font_loader.h
 *
 */

#ifndef LV_FONT_LOADER_H
#define LV_FONT_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

#if LV_USE_FILESYSTEM

LV_EXPORT_API lv_font_t * lv_font_load(const char * fontName);
LV_EXPORT_API lv_font_t* lv_font_load_from_opened_file(lv_fs_file_t *file);
LV_EXPORT_API void lv_font_free(lv_font_t * font);

#endif

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_FONT_LOADER_H*/
