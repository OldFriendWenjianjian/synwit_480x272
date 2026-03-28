#ifndef SYNWIT_SERIAL_DISPLAY_DECLARE_H
#define SYNWIT_SERIAL_DISPLAY_DECLARE_H

#include "synwit_types.h"
#include "utils/serial_display/serial_display_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief 执行串口屏命令
* @param[in]  sd_frame           串口帧数据
* @param[in]  resp_buffer        应答帧缓存区
* @param[in]  resp_buffer_size   应答帧缓存区大小
* @param[in]  always_null        保留参数，当前传NULL
* @return  <0表示出错，>=0表示应答帧数据实际长度（包含帧头，但不含CRC)
*/
EXPORT_API int synwit_sdcmd_run(const uint8_t* sd_frame,
                                uint8_t* resp_buffer,
                                uint32_t resp_buffer_size,
                                void* always_null);

#ifdef __cplusplus
}
#endif
#endif /* SYNWIT_SERIAL_DISPLAY_DECLARE_H */