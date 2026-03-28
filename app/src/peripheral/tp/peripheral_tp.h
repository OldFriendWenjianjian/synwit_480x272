#ifndef PERIPHERAL_TP_H
#define PERIPHERAL_TP_H
#include "SWM341.h"
#include <stdint.h>

typedef void (*i2c_init_func_t)(uint32_t master_clk);
typedef uint8_t (*i2c_start_func_t)(uint8_t addr, uint8_t wait);
typedef void (*i2c_stop_func_t)(uint8_t wait);
typedef uint8_t (*i2c_read_func_t)(uint8_t ack, uint8_t wait);
typedef uint8_t (*i2c_write_func_t)(uint8_t data, uint8_t wait);

typedef struct {
    i2c_init_func_t init;
    i2c_start_func_t start;
    i2c_stop_func_t stop;
    i2c_read_func_t read;
    i2c_write_func_t write;
    
    GPIO_TypeDef *reset_port;
    uint8_t reset_pin;
    
    GPIO_TypeDef *interrupt_port;
    uint8_t interrupt_pin;
    uint8_t irq; 
}i2c_descriptor_t;

/** 触摸状态 */
enum
{
    TP_STATE_RELEASED = 0,
    TP_STATE_PRESSED
};

/** 触摸信息结构体 */
typedef struct
{
    uint16_t x;     /*!< 当前x坐标 */
    uint16_t y;     /*!< 当前y坐标 */
    uint8_t status; /*!< 触摸状态 */
} tp_dev_t;

#define TP_IRQ_MAKE(irq_port, irq_pin)    \
    extern void (*fp_tp_read_points)(void); \
    static GPIO_TypeDef *s_tp_irq_port = irq_port; \
    static uint8_t s_tp_irq_pin = (uint8_t)irq_pin; \
    static uint8_t s_tp_irq = irq_port##_IRQn; \
    void irq_port##_Handler(void) { \
        if (EXTI_State(irq_port, irq_pin)) \
        { \
            EXTI_Clear(irq_port, irq_pin); \
            fp_tp_read_points(); \
        } \
    }
    
extern tp_dev_t tp_dev;

void peripheral_tp_init(const i2c_descriptor_t *i2c_ops);

#endif //PERIPHERAL_TP_H