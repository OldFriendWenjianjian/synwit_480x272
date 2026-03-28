#ifndef SYNWIT_UI_PREFERENCE_H
#define SYNWIT_UI_PREFERENCE_H

#include "lvgl/lvgl.h"
#include "../synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 打开配置文件
* @param[in]  pref_file     文件名
* @param[in]  read_only     只读模式传true，需要修改/追加/删除传false
* @param[in]  always_zero   保留，固定传0
* @return  返回配置对象指针，打开失败则返回NULL
*/
EXPORT_API void* synwit_ui_pref_open(const char* pref_file, bool read_only, uint32_t always_zero);



/**@brief 关闭配置文件
* @param[in]  pref          synwit_ui_pref_open()返回的配置对象指针
*/
EXPORT_API void synwit_ui_pref_close(void* pref);



/**@brief 保存配置到指定文件内
* @param[in]  pref          synwit_ui_pref_open()返回的配置对象指针
* @param[in]  pref_file     需要保存的文件路径
* @return  保存成功返回0，否则返回<0
*/
EXPORT_API int synwit_ui_pref_save(void* pref, const char* pref_file);



/**@brief 读取配置
* @param[in]  pref          synwit_ui_pref_open()返回的配置对象指针
* @param[in]  name          属性名称
* @param[in]  def           当属性名在配置中不存在时，返回的默认值
* @return  返回对应的属性内容
*/
EXPORT_API int synwit_ui_pref_get_int(void* pref, const char* name, int def);
EXPORT_API const char* synwit_ui_pref_get_string(void* pref, const char* name, const char* def);
EXPORT_API const char* synwit_ui_pref_get_value_by_idx(void* pref, uint32_t index);
EXPORT_API const char* synwit_ui_pref_get_key_by_idx(void* pref, uint32_t index);



/**@brief 修改配置
* @param[in]  pref          synwit_ui_pref_open()返回的配置对象指针
* @param[in]  name          属性名称
* @param[in]  value         属性值
* @return  成功返回0，否则返回<0
*/
EXPORT_API int synwit_ui_pref_set_string(void* pref, const char* name, const char* value);
EXPORT_API int synwit_ui_pref_set_int(void* pref, const char* name, int value);



/**@brief 移除配置项
* @param[in]  pref          synwit_ui_pref_open()返回的配置对象指针
* @param[in]  name          要移除的属性名称
*/
EXPORT_API void synwit_ui_pref_remove(void* pref, const char* name);



/**@brief 获取配置项的总数
* @param[in]  pref          synwit_ui_pref_open()返回的配置对象指针
* @return  返回配置项总数
*/
EXPORT_API uint32_t synwit_ui_pref_get_num(void* pref);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SYNWIT_UI_PREFERENCE_H*/