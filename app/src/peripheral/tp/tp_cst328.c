//----------------------------头文件依赖----------------------------//
#include "board.h"
#include "peripheral_tp.h"
#include "lvgl/lvgl.h"

#define TP_PRES_DOWN 		(0x80) //触屏被按下

static i2c_descriptor_t s_i2c_desc;

#define CST328_ADDRESS      0x34
#define TP_TOUCH1           0xD000

//----------------------------本地函数定义----------------------------//
static inline void delay(void)
{
    for (uint32_t i = 0; i < 800; i++)
        __NOP();
}

// TP_IC 写入寄存器
static uint32_t touch_write_regs(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint32_t i;
    uint8_t ack;

    ack = s_i2c_desc.start(CST328_ADDRESS | 0, 1);
    if (ack == 0)
        goto wr_fail;

    ack = s_i2c_desc.write(reg >> 8, 1);
    if (ack == 0)
        goto wr_fail;
    
    ack = s_i2c_desc.write(reg & 0XFF, 1);
    if (ack == 0)
        goto wr_fail;

    for (i = 0; i < len; i++)
    {
        ack = s_i2c_desc.write(buf[i], 1);
        if (ack == 0)
            goto wr_fail;
    }

    s_i2c_desc.stop(1);
    delay();
    return 0;

wr_fail:
    s_i2c_desc.stop(1);
    delay();
    return 1;
}

// TP_IC 读取寄存器
static uint32_t touch_read_regs(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    uint8_t ack;

    ack = s_i2c_desc.start(CST328_ADDRESS | 0, 1);
    if (ack == 0)
        goto rd_fail;

    ack = s_i2c_desc.write(reg >> 8, 1);
    if (ack == 0)
        goto rd_fail;
    
    ack = s_i2c_desc.write(reg & 0XFF, 1);
    if (ack == 0)
        goto rd_fail;

    delay();//此处必须延时等待一会再启动

    ack = s_i2c_desc.start(CST328_ADDRESS | 1, 1); //ReStart
    if (ack == 0)
        goto rd_fail;

    for (i = 0; i < len - 1; i++)
    {
        buf[i] = s_i2c_desc.read(1, 1);
    }
    buf[i] = s_i2c_desc.read(0, 1);

    s_i2c_desc.stop(1);
    delay();
    return 0;

rd_fail:
    s_i2c_desc.stop(1);
    delay();
    return 1;
}

//----------------------------对外接口----------------------------//
/**
  \brief		TP_IC 读取触摸点
  \param [in]	tp_dev_t 
  \return       0 : success    非0: error
  \note    		\
 */
uint8_t cst328_read_points(tp_dev_t *tp_dev)
{
    uint8_t tp_value[5] = {0};
    static tp_dev_t last_tp_dev = {0xFFFF,0xFFFF,TP_STATE_RELEASED};
    
    touch_read_regs(TP_TOUCH1, tp_value, 4);

    if (tp_value[0] == 0x16)
    {
        tp_dev->x = ((uint16_t)(tp_value[1] << 4) | (tp_value[3] >> 4));
        tp_dev->y = ((uint16_t)(tp_value[2] << 4) | (tp_value[3] & 0x0F));
        if ((tp_dev->x > 480) || (tp_dev->y > 480))
        {
            tp_dev->x = last_tp_dev.x;
            tp_dev->y = last_tp_dev.y;
        }
        tp_dev->status = TP_PRES_DOWN;
        last_tp_dev.x = tp_dev->x;
        last_tp_dev.y = tp_dev->y;
        last_tp_dev.status = tp_dev->status;
        return 0;
    }
    else if(tp_value[0] == 0x10)
    {
        tp_dev->x = ((uint16_t)(tp_value[1] << 4) | (tp_value[3] >> 4));
        tp_dev->y = ((uint16_t)(tp_value[2] << 4) | (tp_value[3] & 0x0F));
        if ((tp_dev->x > 480) || (tp_dev->y > 480))
        {
            tp_dev->x = last_tp_dev.x;
            tp_dev->y = last_tp_dev.y;
        }
        tp_dev->status &= ~TP_PRES_DOWN;
        last_tp_dev.x = tp_dev->x;
        last_tp_dev.y = tp_dev->y;
        last_tp_dev.status = tp_dev->status;
        return 0;
    }
    else
    {
        tp_dev->x = last_tp_dev.x;
        tp_dev->y = last_tp_dev.y;
        tp_dev->status &= ~TP_PRES_DOWN;
        return 1;
    }
}


void tp_cst328_init(const i2c_descriptor_t *desc)
{
    uint8_t i = 0;
    uint8_t temp_value = 0;
    
    memcpy(&s_i2c_desc, desc, sizeof(i2c_descriptor_t));
    s_i2c_desc.init(300000);

    GPIO_Init(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin, 0, 0, 1, 0);

    GPIO_Init(s_i2c_desc.reset_port, s_i2c_desc.reset_pin, 1, 1, 0, 0); //复位
    GPIO_ClrBit(s_i2c_desc.reset_port, s_i2c_desc.reset_pin);
    swm_delay_ms(1);
    GPIO_SetBit(s_i2c_desc.reset_port, s_i2c_desc.reset_pin); // 释放复位
    swm_delay_ms(200);

    EXTI_Init(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin, EXTI_BOTH_EDGE); //下降沿触发
    NVIC_EnableIRQ(s_i2c_desc.irq);
    EXTI_Open(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin);
}

void tp_cst328_read_points(void)
{
    cst328_read_points(&tp_dev);
}
