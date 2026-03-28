#ifndef SYNWIT_EZOC_H
#define SYNWIT_EZOC_H

#include <stdlib.h>
#include <stdint.h>
#include "synwit_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EZOC_VT_NUMBER = 0,
    EZOC_VT_STRING,
    EZOC_VT_DATA,
}ezoc_value_type_t;

typedef struct {
    void* data;
    uint32_t data_len;
}ezoc_value_data_t;

typedef struct {
    union {
        double number;
        void* data;
    } v;
    uint32_t data_len;
    ezoc_value_type_t value_type;
}ezoc_value_t;

/* 执行EZOC命令
    参数：
        (IN)    code:       EZOC命令内容，例如: "checkbox1.checked = 1"
        (OUT)   value_type: 描述返回值的数据类型，可以传NULL
        (OUT)   err:        命令执行过程中的错误代码，可以传NULL
    返回值:
        EZOC命令执行结果，统一抽象为double，具体数据类型
        可以根据value_type判断
*/
EXPORT_API double synwit_ezoc_run(const char *code, ezoc_value_type_t *value_type, int *err);


#ifdef __cplusplus
}
#endif

#endif /* SYNWIT_EZOC_H */