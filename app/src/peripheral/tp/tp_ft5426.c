/**
 * @file dev_ft5426.c
 * @author lik
 * @brief ft5426相关配置函数
 * @version 0.1
 * @date 2022-08-29
 *
 * @copyright Copyright (c) 2022
 *
 */

/** 如果需要打印触摸调试信息请定义宏 FT5426_DEBUG */
//#define FT5426_DEBUG

#ifdef FT5426_DEBUG
#define ft5426_debug(...) printf(__VA_ARGS__)
#else
#define ft5426_debug(...) ;
#endif // FT5426_DEBUG

#include "board.h"
#include "peripheral_tp.h"
#include "lv_conf.h"

#define FT5426_ADDR 0x38 /*!< ft5426设备地址 */

#define FT5426_CURPOINT 0x02 /*!< 当前检测到的触摸情况 */
#define FT5426_TP1 0x03   /*!< 第一个触摸点数据地址 */

static i2c_descriptor_t s_i2c_desc;

/**
 * @brief ft5426写寄存器
 *
 * @param reg 要写入的寄存器
 * @param buf 要写入的数组
 * @param len 要写入的数组的长度
 * @return true
 * @return false
 */
static bool ft5426_write_regs(uint8_t reg, uint8_t *buf, uint32_t len)
{
    uint32_t i;
    uint8_t ack;

    ack = s_i2c_desc.start((FT5426_ADDR << 1) | 0, 1);
    if (ack == 0)
        goto wr_fail;
    ack = s_i2c_desc.write(reg, 1);
    if (ack == 0)
        goto wr_fail;
    for (i = 0; i < len; i++)
    {
        ack = s_i2c_desc.write(buf[i], 1);
        if (ack == 0)
            goto wr_fail;
    }

    s_i2c_desc.stop(1);
    for (i = 0; i < CyclesPerUs; i++)
        __NOP();
    return true;

wr_fail:
    s_i2c_desc.stop(1);
    for (i = 0; i < CyclesPerUs; i++)
        __NOP();
    return false;
}

/**
 * @brief ft5426读寄存器
 *
 * @param reg 要读取的寄存器
 * @param buf 要读取的数组
 * @param len 要读取的数组的长度
 * @return true
 * @return false
 */
static bool ft5426_read_regs(uint8_t reg, uint8_t *buf, uint32_t len)
{
    uint32_t i;
    uint8_t ack;

    ack = s_i2c_desc.start((FT5426_ADDR << 1) | 0, 1);
    if (ack == 0)
        goto rd_fail;
    ack = s_i2c_desc.write(reg, 1);
    if (ack == 0)
        goto rd_fail;
    for (i = 0; i < CyclesPerUs; i++)
        __NOP();
    ack = s_i2c_desc.start((FT5426_ADDR << 1) | 1, 1); // ReStart
    if (ack == 0)
        goto rd_fail;
    for (i = 0; i < len - 1; i++)
    {
        buf[i] = s_i2c_desc.read(1, 1);
    }
    buf[i] = s_i2c_desc.read(0, 1);
    s_i2c_desc.stop(1);
    for (i = 0; i < CyclesPerUs; i++)
        __NOP();
    return true;
rd_fail:
    s_i2c_desc.stop(1);
    for (i = 0; i < CyclesPerUs; i++)
        __NOP();
    return false;
}

/**
 * @brief ft5426读取触摸点
 *
 */
static void ft5426_read_points(void)
{
    uint8_t touch_num, buf[4];

    ft5426_read_regs(FT5426_CURPOINT, &touch_num, 1);
    if (touch_num)
    {
        tp_dev.status = TP_STATE_PRESSED;

        ft5426_read_regs(FT5426_TP1, buf, 4); //读取XY坐标值
        tp_dev.x = (((uint16_t)(buf[2] & 0X0F) << 8) + buf[3]);
        tp_dev.y = (((uint16_t)(buf[0] & 0X0F) << 8) + buf[1]);

        ft5426_debug("%d %d\n", tp_dev.x, tp_dev.y);
    }
    else //无触摸点按下
    {
        tp_dev.x = 0xFFFF;
        tp_dev.y = 0xFFFF;
        tp_dev.status = TP_STATE_RELEASED;
    }
}

/**
 * @brief tp初始化
 *
 */
void tp_ft5426_init(const i2c_descriptor_t *desc)
{
    memcpy(&s_i2c_desc, desc, sizeof(i2c_descriptor_t));
    s_i2c_desc.init(400000);
    
    GPIO_Init(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin, 0, 0, 1, 0);

    GPIO_Init(s_i2c_desc.reset_port, s_i2c_desc.reset_pin, 1, 1, 0, 0); // 复位
    GPIO_ClrBit(s_i2c_desc.reset_port, s_i2c_desc.reset_pin);
    for (uint32_t i = 0; i < CyclesPerUs; i++)
        __NOP();
    GPIO_SetBit(s_i2c_desc.reset_port, s_i2c_desc.reset_pin);
    for (uint32_t i = 0; i < CyclesPerUs; i++)
        __NOP();

    EXTI_Init(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin, EXTI_FALL_EDGE); //下降沿触发
    NVIC_EnableIRQ(s_i2c_desc.irq);
    EXTI_Open(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin);
}

void tp_ft5426_read_points(void)
{
    ft5426_read_points();
}