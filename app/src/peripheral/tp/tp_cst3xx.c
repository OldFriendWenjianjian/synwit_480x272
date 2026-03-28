
#include "board.h"
#include "peripheral_tp.h"
#include "lvgl/lvgl.h"

/** 如果需要打印触摸调试信息请定义宏 CST32XX_DEBUG_EN */
//#define CST32XX_DEBUG_EN
#ifdef CST32XX_DEBUG_EN
# define CST32XX_DEBUG(...)    printf(__VA_ARGS__)
#else
# define CST32XX_DEBUG(...)    ;
#endif // CST32XX_DEBUG_EN

#define TP_PRES_DOWN 		(0x80) //触屏被按下

static i2c_descriptor_t s_i2c_desc;

/** 按 8 位地址的方式定义(7 bit 器件地址 + 1 bit 读写控制位) */
#define CST328_ADDRESS              (0x1A << 1) /* default */
#define CST3240_ADDRESS             (0x5A << 1)

#define TP_IIC_ADDR                 CST328_ADDRESS

/* CST3xx 触摸信息寄存器 */
#define TP_DATA_REG                 0xD000

#if 1 /* register address */
/********selfcap register address start *****************/
#define HYN_REG_CAP_POWER_MODE                  0xA5
#define HYN_REG_CAP_POWER_MODE_SLEEP_VALUE      0x03
#define HYN_REG_CAP_FW_VER                      0xA6
#define HYN_REG_CAP_VENDOR_ID                   0xA8
#define HYN_REG_CAP_PROJECT_ID                  0xA9
#define HYN_REG_CAP_CHIP_ID                     0xAA
#define HYN_REG_CAP_CHIP_CHECKSUM               0xAC

#define HYN_REG_CAP_GESTURE_EN                  0xD0
#define HYN_REG_CAP_GESTURE_OUTPUT_ADDRESS      0xD3

#define HYN_REG_CAP_PROXIMITY_EN                0xB0
#define HYN_REG_CAP_PROXIMITY_OUTPUT_ADDRESS    0x01

#define HYN_REG_CAP_ESD_SATURATE                0xE0
/********selfcap register address end *****************/

/********mutcap register address start *****************/
//Myabe change
#define HYN_REG_MUT_ESD_VALUE                   0xD040
#define HYN_REG_MUT_ESD_CHECKSUM                0xD046
#define HYN_REG_MUT_PROXIMITY_EN                0xD04B
#define HYN_REG_MUT_PROXIMITY_OUTPUT_ADDRESS    0xD04B
#define HYN_REG_MUT_GESTURE_EN                  0xD04C
#define HYN_REG_MUT_GESTURE_OUTPUT_ADDRESS      0xD04C

//workmode
#define HYN_REG_MUT_DEBUG_INFO_MODE             0xD101
#define HYN_REG_MUT_RESET_MODE                  0xD102
#define HYN_REG_MUT_DEBUG_RECALIBRATION_MODE    0xD104
#define HYN_REG_MUT_DEEP_SLEEP_MODE             0xD105
#define HYN_REG_MUT_DEBUG_POINT_MODE            0xD108
#define HYN_REG_MUT_NORMAL_MODE                 0xD109

#define HYN_REG_MUT_DEBUG_RAWDATA_MODE          0xD10A
#define HYN_REG_MUT_DEBUG_DIFF_MODE             0xD10D
#define HYN_REG_MUT_DEBUG_FACTORY_MODE          0xD119
#define HYN_REG_MUT_DEBUG_FACTORY_MODE_2        0xD120

//debug info
/****************HYN_REG_MUT_DEBUG_INFO_MODE address start***********/
#define HYN_REG_MUT_DEBUG_INFO_IC_CHECKSUM             0xD208
#define HYN_REG_MUT_DEBUG_INFO_FW_VERSION              0xD204
#define HYN_REG_MUT_DEBUG_INFO_IC_TYPE                 0xD202
#define HYN_REG_MUT_DEBUG_INFO_PROJECT_ID              0xD200
#define HYN_REG_MUT_DEBUG_INFO_BOOT_TIME               0xD1FC
#define HYN_REG_MUT_DEBUG_INFO_RES_Y                   0xD1FA
#define HYN_REG_MUT_DEBUG_INFO_RES_X                   0xD1F8
#define HYN_REG_MUT_DEBUG_INFO_KEY_NUM                 0xD1F7
#define HYN_REG_MUT_DEBUG_INFO_TP_NRX                  0xD1F6
#define HYN_REG_MUT_DEBUG_INFO_TP_NTX                  0xD1F4

/****************HYN_REG_MUT_DEBUG_INFO_MODE address end***********/
#define HYN_WORK_MODE_NORMAL     0
#define HYN_WORK_MODE_FACTORY    1
#define HYN_WORK_MODE_RAWDATA    2
#define HYN_WORK_MODE_DIFF       3
#define HYN_WORK_MODE_UPDATE     4
/********mutcap register address end *****************/
#endif

static uint8_t cst3xx_read_points(tp_dev_t *tp_dev);
static uint32_t touch_write_regs(uint32_t reg, uint8_t *buf, uint8_t len);
static uint32_t touch_read_regs(uint32_t reg, uint8_t *buf, uint8_t len);

void tp_cst3xx_read_points(void)
{
    cst3xx_read_points(&tp_dev);
}

void tp_cst3xx_init(const i2c_descriptor_t *desc)
{
    uint8_t temp_value[4] = {0};
    
    memcpy(&s_i2c_desc, desc, sizeof(i2c_descriptor_t));
    s_i2c_desc.init(100 * 1000);

    GPIO_Init(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin, 0, 0, 1, 0);

    GPIO_Init(s_i2c_desc.reset_port, s_i2c_desc.reset_pin, 1, 1, 0, 0); //复位
    GPIO_SetBit(s_i2c_desc.reset_port, s_i2c_desc.reset_pin);
    swm_delay_ms(10);
    GPIO_ClrBit(s_i2c_desc.reset_port, s_i2c_desc.reset_pin);
    swm_delay_ms(10);
    GPIO_SetBit(s_i2c_desc.reset_port, s_i2c_desc.reset_pin); // 释放复位
    swm_delay_ms(200);
    
    /* 进入 debug 模式 */
    touch_write_regs(HYN_REG_MUT_DEBUG_INFO_MODE, temp_value, 0);
    /* 读取固件信息 */
    touch_read_regs(HYN_REG_MUT_DEBUG_INFO_BOOT_TIME, temp_value, 4);
    /* 高位为 固件校验码 == 0xCACA, 低位为 Bootloader 时间 */
    if (temp_value[3] == 0xCA && temp_value[2] == 0xCA)
    {
        /* IC_Type / Project_ID */
        touch_read_regs(HYN_REG_MUT_DEBUG_INFO_FW_VERSION, temp_value, 4);
        CST32XX_DEBUG("CST3xx [FW_info] is true!, type/id = [%d][%d][%d][%d]\r\n", temp_value[0], temp_value[1], temp_value[2], temp_value[3]);
    }
    /* 进入 normal 模式 */
    touch_write_regs(HYN_REG_MUT_NORMAL_MODE, temp_value, 0);

    EXTI_Init(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin, EXTI_FALL_EDGE); //下降沿触发
    NVIC_EnableIRQ(s_i2c_desc.irq);
    EXTI_Open(s_i2c_desc.interrupt_port, s_i2c_desc.interrupt_pin);
}

/**
  \brief		读取触摸点
  \param [in]	tp_dev_t 
  \return       0 : success    非0: error
  \note    		\
 */
static uint8_t cst3xx_read_points(tp_dev_t *tp_dev)
{
    uint8_t tp_value[7] = {0};
    uint8_t touch_points = 0;
    
    if (touch_read_regs(TP_DATA_REG, tp_value, 7))
    {
        CST32XX_DEBUG("touch_read_regs IIC addr invalid!\r\n");
        return 1; /* 从机地址无 ACK  */
    }
    /*
    CST32XX_DEBUG("\r\n");
    for (uint32_t i = 0; i < sizeof(tp_value) / sizeof(tp_value[0]); ++i)
        CST32XX_DEBUG("tp_val[%d] = [%x], ", i, tp_value[i]);
    CST32XX_DEBUG("\r\n");
    */
    uint8_t data_valid = 0;
    if (tp_value[6] != 0xAB || tp_value[0] == 0xAB)
    {
        CST32XX_DEBUG("TP data invalid!\r\n");
        data_valid = 1; /* 数据无效 */
    }
    /* 发送同步命令 0xD000AB, 结束读取触摸 */
    uint8_t sync_reg = 0xAB;
    if (touch_write_regs(TP_DATA_REG, &sync_reg, 1) || data_valid > 0)
    {
        CST32XX_DEBUG("touch_write_regs IIC addr invalid or data invalid[%d]!\r\n", data_valid);
        return 2; /* 从机地址无 ACK*/
    }
    /* 手指触摸个数 */
    touch_points = tp_value[5] & 0x0F;
    if(touch_points == 0 || touch_points > 5)
    {
        tp_dev->status &= ~TP_PRES_DOWN;
        return 3;
    }
    /* 边界值 */
    uint16_t lcd_w = 480;
    uint16_t lcd_h = 480;
    lv_disp_t *disp = lv_disp_get_default();
    if(disp) {
        lcd_w = disp->driver.hor_res;
        lcd_h = disp->driver.ver_res;
    }
    /* 目前仅实现对第 1 个手指的读取 */
    if ((tp_value[0] & 0x0F) == 0x06) /* 手指状态:按下(0x06) */
    {
        tp_dev->status |= TP_PRES_DOWN;
    }
    else
    {
        tp_dev->status &= ~TP_PRES_DOWN;
    }

    uint16_t temp_l = ( ( (uint16_t)(tp_value[1] & 0xFF) ) << 4) | ( (tp_value[3] >> 4) & 0x0F );
    uint16_t temp_h = ( ( (uint16_t)(tp_value[2] & 0xFF) ) << 4) | ( tp_value[3] & 0x0F );
    CST32XX_DEBUG("[%d, %d]\n", temp_l, temp_h);
    
    /* 坐标值有效性判断 */
    if (temp_l > lcd_w || temp_h > lcd_h)
    {
        tp_dev->status &= ~TP_PRES_DOWN;
        return 4;
    }
    /* X/Y 交换 
    temp_l = temp_l ^ temp_h;
    temp_h = temp_l ^ temp_h;
    temp_l = temp_l ^ temp_h;
    */
    
    /* X/Y 基准偏移 */
#if (CST328_ADDRESS == TP_IIC_ADDR)
    tp_dev->x = /*lcd_w -*/ temp_l;
    tp_dev->y = /*lcd_h -*/ temp_h;
    
#elif (CST3240_ADDRESS == TP_IIC_ADDR)
    tp_dev->x = /*lcd_w -*/ temp_l;
    tp_dev->y = lcd_h - temp_h;
#endif
    return 0;
}

static inline void delay(void)
{
    for (uint32_t i = 0; i < 800; i++) __NOP();
}

static uint32_t touch_write_regs(uint32_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    uint8_t ack;

    ack = s_i2c_desc.start(TP_IIC_ADDR | 0, 1);
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

static uint32_t touch_read_regs(uint32_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    uint8_t ack;

    ack = s_i2c_desc.start(TP_IIC_ADDR | 0, 1);
    if (ack == 0)
        goto rd_fail;

    ack = s_i2c_desc.write(reg >> 8, 1);
    if (ack == 0)
        goto rd_fail;
    
    ack = s_i2c_desc.write(reg & 0XFF, 1);
    if (ack == 0)
        goto rd_fail;

    delay(); //此处必须延时等待一会再启动

    ack = s_i2c_desc.start(TP_IIC_ADDR | 1, 1); //ReStart
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
