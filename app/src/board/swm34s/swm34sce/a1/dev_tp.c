#include <string.h>
#include "SWM341.h"
#include "board.h"
#include "tp/peripheral_tp.h"
#include "swm34s/common/io_i2c/dev_io_i2c.h"

__attribute__((weak)) void tp_read_points(void){}

#define TP_GPIO_RST 		GPIOM
#define TP_PIN_RST 			PIN4

#define TP_GPIO_SCL 		GPIOM
#define TP_PIN_SCL 			PIN2

#define TP_GPIO_SDA 		GPIOM
#define TP_PIN_SDA 			PIN3

TP_IRQ_MAKE(GPIOM, PIN5);

static void i2c_init(uint32_t master_clk)
{
    GPIO_Init(TP_GPIO_SCL, TP_PIN_SCL, 1, 1, 0, 0);
    GPIO_Init(TP_GPIO_SDA, TP_PIN_SDA, 1, 1, 0, 0);
    GPIO_AtomicSetBit(TP_GPIO_SCL, TP_PIN_SCL);
    GPIO_AtomicSetBit(TP_GPIO_SDA, TP_PIN_SDA);
}

static uint8_t i2c_start(uint8_t addr, uint8_t wait)
{
    return io_i2c_start(addr, wait);
}

static void i2c_stop(uint8_t wait)
{
    io_i2c_stop(wait);
}

static uint8_t i2c_read(uint8_t ack, uint8_t wait)
{
    return io_i2c_read(ack, wait);
}

static uint8_t i2c_write(uint8_t data, uint8_t wait)
{
    return io_i2c_write(data, wait);
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

#include "swm34s/common/io_i2c/dev_io_i2c.c"
