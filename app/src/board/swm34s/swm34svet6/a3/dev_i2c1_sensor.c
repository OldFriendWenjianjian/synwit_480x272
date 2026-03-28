#include "dev_i2c1_sensor.h"

#include "SWM341.h"
#include "board.h"

#define SENSOR_I2C_INDEX        I2C1
#define SENSOR_PORT_SCL         PORTA
#define SENSOR_I2C_SCL_PIN      PIN7
#define SENSOR_I2C_SCL_PIN_FUNC PORTA_PIN7_I2C1_SCL
#define SENSOR_PORT_SDA         PORTA
#define SENSOR_I2C_SDA_PIN      PIN6
#define SENSOR_I2C_SDA_PIN_FUNC PORTA_PIN6_I2C1_SDA

static bool s_i2c1_sensor_inited = false;
static uint32_t s_i2c1_sensor_clk = 0U;

static void dev_i2c1_sensor_bus_init(uint32_t master_clk)
{
    I2C_InitStructure init_struct;

    PORT_Init(SENSOR_PORT_SCL, SENSOR_I2C_SCL_PIN, SENSOR_I2C_SCL_PIN_FUNC, 1);
    SENSOR_PORT_SCL->OPEND |= (1U << SENSOR_I2C_SCL_PIN);
    SENSOR_PORT_SCL->PULLU |= (1U << SENSOR_I2C_SCL_PIN);

    PORT_Init(SENSOR_PORT_SDA, SENSOR_I2C_SDA_PIN, SENSOR_I2C_SDA_PIN_FUNC, 1);
    SENSOR_PORT_SDA->OPEND |= (1U << SENSOR_I2C_SDA_PIN);
    SENSOR_PORT_SDA->PULLU |= (1U << SENSOR_I2C_SDA_PIN);

    init_struct.Master = 1;
    init_struct.MstClk = master_clk;
    init_struct.Addr10b = 0;
    init_struct.SlvAddr = 0;
    init_struct.SlvAddrMsk = 0;
    init_struct.TXEmptyIEn = 0;
    init_struct.RXNotEmptyIEn = 0;
    init_struct.SlvSTADetIEn = 0;
    init_struct.SlvSTODetIEn = 0;

    I2C_Init(SENSOR_I2C_INDEX, &init_struct);
    I2C_Open(SENSOR_I2C_INDEX);
}

static void dev_i2c1_sensor_ensure_init(void)
{
    if(s_i2c1_sensor_inited == false) {
        dev_i2c1_sensor_init(DEV_I2C1_SENSOR_DEFAULT_CLOCK);
    }
}

static uint8_t dev_i2c1_sensor_make_addr(uint8_t addr7, bool is_read)
{
    return (uint8_t)((addr7 << 1U) | (is_read ? 1U : 0U));
}

static void dev_i2c1_sensor_stop(void)
{
    I2C_Stop(SENSOR_I2C_INDEX, 1);
}

void dev_i2c1_sensor_init(uint32_t master_clk)
{
    if(master_clk == 0U) {
        master_clk = DEV_I2C1_SENSOR_DEFAULT_CLOCK;
    }

    if((s_i2c1_sensor_inited == true) && (s_i2c1_sensor_clk == master_clk)) {
        return;
    }

    dev_i2c1_sensor_bus_init(master_clk);
    s_i2c1_sensor_clk = master_clk;
    s_i2c1_sensor_inited = true;
}

bool dev_i2c1_sensor_probe(uint8_t addr7)
{
    bool ret;

    dev_i2c1_sensor_ensure_init();

    ret = (I2C_Start(SENSOR_I2C_INDEX, dev_i2c1_sensor_make_addr(addr7, false), 1) == 1U);
    dev_i2c1_sensor_stop();

    return ret;
}

bool dev_i2c1_sensor_write(uint8_t addr7, const uint8_t *data, uint32_t len)
{
    uint32_t i;

    if((len > 0U) && (data == 0)) {
        return false;
    }

    if(len == 0U) {
        return dev_i2c1_sensor_probe(addr7);
    }

    dev_i2c1_sensor_ensure_init();

    if(I2C_Start(SENSOR_I2C_INDEX, dev_i2c1_sensor_make_addr(addr7, false), 1) == 0U) {
        dev_i2c1_sensor_stop();
        return false;
    }

    for(i = 0U; i < len; i++) {
        if(I2C_Write(SENSOR_I2C_INDEX, data[i], 1) == 0U) {
            dev_i2c1_sensor_stop();
            return false;
        }
    }

    dev_i2c1_sensor_stop();
    return true;
}

bool dev_i2c1_sensor_read(uint8_t addr7, uint8_t *data, uint32_t len)
{
    uint32_t i;

    if((data == 0) || (len == 0U)) {
        return false;
    }

    dev_i2c1_sensor_ensure_init();

    if(I2C_Start(SENSOR_I2C_INDEX, dev_i2c1_sensor_make_addr(addr7, true), 1) == 0U) {
        dev_i2c1_sensor_stop();
        return false;
    }

    for(i = 0U; i < len; i++) {
        data[i] = I2C_Read(SENSOR_I2C_INDEX, (i + 1U) < len ? 1U : 0U, 1);
    }

    dev_i2c1_sensor_stop();
    return true;
}

bool dev_i2c1_sensor_write_read(uint8_t addr7,
                                const uint8_t *tx_data,
                                uint32_t tx_len,
                                uint8_t *rx_data,
                                uint32_t rx_len)
{
    uint32_t i;

    if(((tx_len > 0U) && (tx_data == 0)) || ((rx_len > 0U) && (rx_data == 0))) {
        return false;
    }

    if(rx_len == 0U) {
        return dev_i2c1_sensor_write(addr7, tx_data, tx_len);
    }

    if(tx_len == 0U) {
        return dev_i2c1_sensor_read(addr7, rx_data, rx_len);
    }

    dev_i2c1_sensor_ensure_init();

    if(I2C_Start(SENSOR_I2C_INDEX, dev_i2c1_sensor_make_addr(addr7, false), 1) == 0U) {
        dev_i2c1_sensor_stop();
        return false;
    }

    for(i = 0U; i < tx_len; i++) {
        if(I2C_Write(SENSOR_I2C_INDEX, tx_data[i], 1) == 0U) {
            dev_i2c1_sensor_stop();
            return false;
        }
    }

    if(I2C_Start(SENSOR_I2C_INDEX, dev_i2c1_sensor_make_addr(addr7, true), 1) == 0U) {
        dev_i2c1_sensor_stop();
        return false;
    }

    for(i = 0U; i < rx_len; i++) {
        rx_data[i] = I2C_Read(SENSOR_I2C_INDEX, (i + 1U) < rx_len ? 1U : 0U, 1);
    }

    dev_i2c1_sensor_stop();
    return true;
}
