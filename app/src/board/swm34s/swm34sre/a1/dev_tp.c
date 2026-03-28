#include <string.h>
#include "SWM341.h"
#include "board.h"
#include "tp/peripheral_tp.h"

__attribute__((weak)) void tp_read_points(void){}

#define TP_I2C_INDEX        I2C1

#define TP_GPIO_RST 		GPIOB
#define TP_PIN_RST 			PIN11

#define TP_PORT_SCL 		PORTC
#define TP_I2C_SCL_PIN      PIN5
#define TP_I2C_SCL_PIN_FUNC PORTC_PIN5_I2C1_SCL

#define TP_PORT_SDA 		PORTC
#define TP_I2C_SDA_PIN      PIN4
#define TP_I2C_SDA_PIN_FUNC PORTC_PIN4_I2C1_SDA

TP_IRQ_MAKE(GPIOD, PIN0);

static void i2c_init(uint32_t master_clk)
{
    I2C_InitStructure I2C_initStruct;

    PORT_Init(TP_PORT_SCL, TP_I2C_SCL_PIN, TP_I2C_SCL_PIN_FUNC, 1); // GPIOA.1配置为I2C0 SCL引脚
    TP_PORT_SCL->OPEND |= (1 << TP_I2C_SCL_PIN);
    TP_PORT_SCL->PULLU |= (1 << TP_I2C_SCL_PIN);                    //必须使能上拉，用于模拟开漏
    PORT_Init(TP_PORT_SDA, TP_I2C_SDA_PIN, TP_I2C_SDA_PIN_FUNC, 1); // GPIOA.0配置为I2C0 SDA引脚
    TP_PORT_SDA->OPEND |= (1 << TP_I2C_SDA_PIN);
    TP_PORT_SDA->PULLU |= (1 << TP_I2C_SDA_PIN); //必须使能上拉，用于模拟开漏

    I2C_initStruct.Master = 1;
    I2C_initStruct.MstClk = master_clk;
    I2C_initStruct.Addr10b = 0;
    I2C_initStruct.TXEmptyIEn = 0;
    I2C_initStruct.RXNotEmptyIEn = 0;
    I2C_Init(TP_I2C_INDEX, &I2C_initStruct);
    I2C_Open(TP_I2C_INDEX);
}

static uint8_t i2c_start(uint8_t addr, uint8_t wait)
{
    return I2C_Start(TP_I2C_INDEX, addr, wait);
}

static void i2c_stop(uint8_t wait)
{
    I2C_Stop(TP_I2C_INDEX, wait);
}

static uint8_t i2c_read(uint8_t ack, uint8_t wait)
{
    return I2C_Read(TP_I2C_INDEX, ack, wait);
}

static uint8_t i2c_write(uint8_t data, uint8_t wait)
{
    return I2C_Write(TP_I2C_INDEX, data, wait);
}

void board_tp_init(void)
{
    i2c_descriptor_t i2c_desc;
    memset(&i2c_desc, 0, sizeof(i2c_descriptor_t));
    
    i2c_desc.init = i2c_init;
    i2c_desc.start = i2c_start;
    i2c_desc.stop = i2c_stop;
    i2c_desc.read = i2c_read;
    i2c_desc.write = i2c_write;
    
    i2c_desc.reset_port = TP_GPIO_RST;
    i2c_desc.reset_pin = TP_PIN_RST;
    
    i2c_desc.interrupt_port = s_tp_irq_port;
    i2c_desc.interrupt_pin = s_tp_irq_pin;
    i2c_desc.irq = s_tp_irq;
    
    peripheral_tp_init(&i2c_desc);
}