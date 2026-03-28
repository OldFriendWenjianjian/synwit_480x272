/** 如果需要更新触摸配置请定义宏 UPDATE_CFG */
//#define UPDATE_CFG
/** 如果需要打印触摸调试信息请定义宏 GT9X_DEBUG */
//#define GT9X_DEBUG

#ifdef GT9X_DEBUG
#define gt9x_debug(...) printf(__VA_ARGS__)
#else
#define gt9x_debug(...) ;
#endif // GT9X_DEBUG

#include "SWM341.h"
#include "board.h"
#include "peripheral_tp.h"
#include "lv_conf.h"
#include "lvgl/lvgl.h"

static i2c_descriptor_t s_i2c_desc;

extern lv_coord_t lv_disp_get_ver_res(lv_disp_t * disp);

#ifdef UPDATE_CFG
/**  配置文件的版本号(新下发的配置版本号大于原版本，或等于原版本号但配置内容有变化时保存，
     版本号版本正常范围：'A'~'Z',发送 0x00 则将版本号初始化为'A') */
static uint8_t gt9x_cfg_tab[] = {
    0x00,       /*!< config_version */
    0xE0, 0x01, /*!< x output max */
    0x10, 0x01, /*!< y output max */
    0x05,       /*!< touch number */
    0x3D, 0x00, /*!< module switch */
    0x02, 0x08, 0x1E, 0x08, 0x50, 0x3C, 0x0F, 0x05,
    0x00, 0x00, 0xFF, 0x67, 0x50, 0x00, 0x00, 0x18,
    0x1A, 0x1E, 0x14, 0x89, 0x28, 0x0A, 0x30, 0x2E,
    0xBB, 0x0A, 0x03, 0x00, 0x00, 0x02, 0x33, 0x1D,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32,
    0x00, 0x00, 0x2A, 0x1C, 0x5A, 0x94, 0xC5, 0x02,
    0x07, 0x00, 0x00, 0x00, 0xB5, 0x1F, 0x00, 0x90,
    0x28, 0x00, 0x77, 0x32, 0x00, 0x62, 0x3F, 0x00,
    0x52, 0x50, 0x00, 0x52, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0F, 0x0F, 0x03, 0x06, 0x10,
    0x42, 0xF8, 0x0F, 0x14, 0x00, 0x00, 0x00, 0x00,
    0x1A, 0x18, 0x16, 0x14, 0x12, 0x10, 0x0E, 0x0C,
    0x0A, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x28,
    0x24, 0x22, 0x20, 0x1F, 0x1E, 0x1D, 0x0E, 0x0C,
    0x0A, 0x08, 0x06, 0x05, 0x04, 0x02, 0x00, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#endif

#define GT9x_ADDR 0x5D /*!< gt9x设备地址 */

#define GT9x_CTRL 0x8040  /*!< gt9x控制寄存器 */
#define GT9x_CFGS 0x8047  /*!< gt9x配置寄存器 */
#define GT9x_CHECK 0x80FF /*!< gt9x校验和寄存器 */
#define GT9x_PID 0x8140   /*!< gt9x产品ID寄存器 */

#define GT9x_GSTID 0x814E /*!< 当前检测到的触摸情况 */
#define GT9x_TP1 0x8150   /*!< 第一个触摸点数据地址 */
#define GT9x_TP2 0x8158   /*!< 第二个触摸点数据地址 */
#define GT9x_TP3 0x8160   /*!< 第三个触摸点数据地址 */
#define GT9x_TP4 0x8168   /*!< 第四个触摸点数据地址 */
#define GT9x_TP5 0x8170   /*!< 第五个触摸点数据地址 */

/**
 * @brief gt9x写寄存器
 *
 * @param reg 要写入的寄存器
 * @param buf 要写入的数组
 * @param len 要写入的数组的长度
 * @return true
 * @return false
 */
static bool gt9x_write_regs(uint16_t reg, uint8_t *buf, uint32_t len)
{
    uint32_t i;
    uint8_t ack;

    ack = s_i2c_desc.start((GT9x_ADDR << 1) | 0, 1);
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
    for (i = 0; i < CyclesPerUs/10; i++)
        __NOP();
    return true;

wr_fail:
    s_i2c_desc.stop(1);
    for (i = 0; i < CyclesPerUs/10; i++)
        __NOP();
    return false;
}

/**
 * @brief gt9x读寄存器
 *
 * @param reg 要读取的寄存器
 * @param buf 要读取的数组
 * @param len 要读取的数组的长度
 * @return true
 * @return false
 */
static bool gt9x_read_regs(uint16_t reg, uint8_t *buf, uint32_t len)
{
    uint32_t i;
    uint8_t ack;

    ack = s_i2c_desc.start((GT9x_ADDR << 1) | 0, 1);
    if (ack == 0)
        goto rd_fail;
    ack = s_i2c_desc.write(reg >> 8, 1);
    if (ack == 0)
        goto rd_fail;
    ack = s_i2c_desc.write(reg & 0XFF, 1);
    if (ack == 0)
        goto rd_fail;
    for (i = 0; i < CyclesPerUs/10; i++)
        __NOP();
    ack = s_i2c_desc.start((GT9x_ADDR << 1) | 1, 1); // ReStart
    if (ack == 0)
        goto rd_fail;
    for (i = 0; i < len - 1; i++)
    {
        buf[i] = s_i2c_desc.read(1, 1);
    }
    buf[i] = s_i2c_desc.read(0, 1);
    s_i2c_desc.stop(1);
    for (i = 0; i < CyclesPerUs/10; i++)
        __NOP();
    return true;
rd_fail:
    s_i2c_desc.stop(1);
    for (i = 0; i < CyclesPerUs/10; i++)
        __NOP();
    return false;
}

/**
 * @brief gt9x读取触摸点
 *
 */
static void gt9x_read_points(void)
{
    uint8_t touch_status, touch_num, temp, buf[5];

    gt9x_read_regs(GT9x_GSTID, &touch_status, 1);
    touch_num = touch_status & 0x0F;
    if ((touch_status & 0x80) && (touch_num < 6))
    {
        temp = 0;
        gt9x_write_regs(GT9x_GSTID, &temp, 1); // 清除READY标志
    }
    if (touch_num && (touch_num < 6))
    {
        tp_dev.status = TP_STATE_PRESSED;

        gt9x_read_regs(GT9x_TP1, buf, 4); //读取XY坐标值
        tp_dev.x = (((uint16_t)buf[1] << 8) + buf[0]);
        tp_dev.y = (((uint16_t)buf[3] << 8) + buf[2]);
        
        lv_disp_t *disp = lv_disp_get_default();
        if(disp) {
            tp_dev.y = disp->driver.ver_res - tp_dev.y;
        }
        gt9x_debug("%d %d\n", tp_dev.x, tp_dev.y);
    }
    if ((touch_status & 0x8F) == 0x80) //无触摸点按下
    {
        tp_dev.x = 0xFFFF;
        tp_dev.y = 0xFFFF;
        tp_dev.status = TP_STATE_RELEASED;
    }
}

#ifdef UPDATE_CFG
/**
 * @brief 更新触摸配置固件
 *
 * @return true
 * @return false
 */
static bool gt9x_update_cfg(void)
{
    uint8_t buf[2];
    uint8_t i = 0;

    /* set x range */
    gt9x_cfg_tab[2] = (uint8_t)(HOR_RES >> 8);
    gt9x_cfg_tab[1] = (uint8_t)(HOR_RES & 0xff);

    /* set y range */
    gt9x_cfg_tab[4] = (uint8_t)(VER_RES >> 8);
    gt9x_cfg_tab[3] = (uint8_t)(VER_RES & 0xff);

    /* change x y */
    // gt9x_cfg_tab[6] ^= (1 << 3);

    /* change int trig type */
    /* FLAG_INT_FALL_RX */
    gt9x_cfg_tab[6] &= 0xFC;
    gt9x_cfg_tab[6] |= 0x01;
    /* FLAG_RDONLY */
    // gt9x_cfg_tab[6] &= 0xFC;
    // gt9x_cfg_tab[6] |= 0x03;

    if (gt9x_write_regs(GT9x_CFGS, gt9x_cfg_tab, sizeof(gt9x_cfg_tab)) != 0) /* send config */
    {
        gt9x_debug("send config failed\n");
        return false;
    }

    buf[0] = 0;
    for (i = 0; i < sizeof(gt9x_cfg_tab); i++)
        buf[0] += gt9x_cfg_tab[i];
    buf[0] = (~buf[0]) + 1;
    buf[1] = 1;
    gt9x_write_regs(GT9x_CHECK, buf, 2);
    for (uint32_t i = 0; i < CyclesPerUs*1000*10; i++)
        __NOP();

    return true;
}
#endif

/**
 * @brief tp初始化
 *
 */
void tp_gt9x_init(const i2c_descriptor_t *desc)
{
    uint8_t temp[5] = {0};

    memcpy(&s_i2c_desc, desc, sizeof(i2c_descriptor_t));
    
    s_i2c_desc.init(400000);
    GPIO_Init(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin, 0, 0, 1, 0); // 输入，开启下拉。复位时INT为低，选择0xBA作为地址

    GPIO_Init(s_i2c_desc.reset_port, s_i2c_desc.reset_pin, 1, 1, 0, 0); // gt9x复位
    GPIO_ClrBit(s_i2c_desc.reset_port, s_i2c_desc.reset_pin);
    for (uint32_t i = 0; i < CyclesPerUs; i++)
        __NOP();
    GPIO_SetBit(s_i2c_desc.reset_port, s_i2c_desc.reset_pin);
    for (uint32_t i = 0; i < CyclesPerUs; i++)
        __NOP();

    temp[0] = 0x02;
    gt9x_write_regs(GT9x_CTRL, temp, 1); // 软复位
    for (uint32_t i = 0; i < CyclesPerUs*10; i++)
        __NOP();

    gt9x_read_regs(GT9x_PID, temp, 4);
    gt9x_debug("touch product id:%s\n", temp);
#ifdef UPDATE_CFG
    /* 更新触摸芯片配置 */
    gt9x_update_cfg();
#endif

    EXTI_Init(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin, EXTI_FALL_EDGE); //下降沿触发
    NVIC_EnableIRQ(s_i2c_desc.irq);
    EXTI_Open(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin);
}

void tp_gt9x_read_points(void)
{
    gt9x_read_points();
}