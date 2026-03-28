/**
 *******************************************************************************************************************************************
 * @file        LCD_JLT35002A_V2.c
 * @brief       TFT-LCD 模组名 : JLT35002A-V2(晶力泰), 屏内 COG 芯片型号 : ILI9488, 以 RGB 模式进行驱动
 * @details     /
 * @since       Change Logs:
 * Date         Author       Notes
 * 2022-12-08   lzh          the first version
 * 2023-02-09   lzh          add the way of SPI-Line 3 / 4 
 * @remark      即使个别不同型号的屏内 COG 芯片型号相同, 但由于模组不同, 最好还是向屏幕模组厂索求初始化代码提供参考.
 *
 * @warning     强烈注意 : JLT35002A-V3(晶力泰), 屏内 COG 芯片型号 : ST7796S , 这款不支持 RGB 模式,
 * 而且是 COG 支持, 但屏模组不支持(踩坑预警), 所以调试屏幕时尽量向屏幕模组供应商寻求支持.
 *******************************************************************************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS WITH CODING INFORMATION
 * REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME. AS A RESULT, SYNWIT SHALL NOT BE HELD LIABLE
 * FOR ANY DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION CONTAINED HEREIN IN CONN-
 * -ECTION WITH THEIR PRODUCTS.
 *
 * @copyright   2012 Synwit Technology
 *******************************************************************************************************************************************
 */
#include "SWM341.h"
#include "swm34s/common/systick/dev_systick.h"

/* 仅 SWM34SCET6_Pin48 Demo 板有预留此款屏幕接口 */

/* ms 级延时 */
#define systick_delay_ms(ms)			swm_delay_ms(ms)

/*******************************************************************************************************************************************
 * Private Define
 *******************************************************************************************************************************************/
/** 初始化 SPI 所用端口数 : 3 or 4 (与 IM[0~2] 相关) */
#define SPI_LINE_NUM            4 /* 3 or 4 */

/* 仅四线 SPI 初始化时使用 */
#define LCD_GPIO_RS             GPIOM
#define LCD_PIN_RS              PIN4

/* 初始化必须使用的端口 */
#define LCD_GPIO_RST            GPIOM
#define LCD_PIN_RST             PIN1

#define LCD_GPIO_CS             GPIOD
#define LCD_PIN_CS              PIN2

#define LCD_GPIO_SCK            GPIOM
#define LCD_PIN_SCK             PIN2

#define LCD_GPIO_SDA            GPIOM
#define LCD_PIN_SDA             PIN3


/* 操作宏 */
#if     1 /* 库函数 */
    /** 获取 IO 输入(须保证对应 IO 为 GPIO 输入状态, 否则操作无效) */
    #define IO_GET(GPIOx, PINx)             GPIO_GetBits(GPIOx, PINx)
    /** IO 置低(须保证对应 IO 为 GPIO 输出状态, 否则操作无效) */
    #define IO_SET_L(GPIOx, PINx)           GPIO_AtomicClrBit(GPIOx, PINx)
    /** IO 置高(须保证对应 IO 为 GPIO 输出状态, 否则操作无效) */
    #define IO_SET_H(GPIOx, PINx)           GPIO_AtomicSetBit(GPIOx, PINx)

#else /* 寄存器 */
    /** 获取 IO 输入(须保证对应 IO 为 GPIO 输入状态, 否则操作无效) */
    #define IO_GET(GPIOx, PINx)             ( ( ( (GPIOx)->IDR ) >> (PINx) ) & 0x01 )
    /** IO 置低(须保证对应 IO 为 GPIO 输出状态, 否则操作无效) */
    #define IO_SET_L(GPIOx, PINx)           ( *( ( &( (GPIOx)->DATAPIN0 ) ) + (PINx) ) = 0 )
    /** IO 置高(须保证对应 IO 为 GPIO 输出状态, 否则操作无效) */
    #define IO_SET_H(GPIOx, PINx)           ( *( ( &( (GPIOx)->DATAPIN0 ) ) + (PINx) ) = 1 )

#endif

#define H_RST()         IO_SET_H(LCD_GPIO_RST, LCD_PIN_RST)
#define L_RST()         IO_SET_L(LCD_GPIO_RST, LCD_PIN_RST)

#define H_CS()          IO_SET_H(LCD_GPIO_CS, LCD_PIN_CS)
#define L_CS()          IO_SET_L(LCD_GPIO_CS, LCD_PIN_CS)

#define H_SCK()         IO_SET_H(LCD_GPIO_SCK, LCD_PIN_SCK)
#define L_SCK()         IO_SET_L(LCD_GPIO_SCK, LCD_PIN_SCK)

#define H_SDA()         IO_SET_H(LCD_GPIO_SDA, LCD_PIN_SDA)
#define L_SDA()         IO_SET_L(LCD_GPIO_SDA, LCD_PIN_SDA)

#define H_RS()          IO_SET_H(LCD_GPIO_RS, LCD_PIN_RS)
#define L_RS()          IO_SET_L(LCD_GPIO_RS, LCD_PIN_RS)

/*******************************************************************************************************************************************
 * Private Function
 *******************************************************************************************************************************************/
static inline void spi_delay_time(void)
{
    for (unsigned int i = 0; i < CyclesPerUs; ++i) __NOP();
}

static void spi_send_byte(unsigned char data)
{
    for (unsigned char i = 0; i < 8; ++i)
    {
        if (data & 0x80)
        {
            H_SDA();
        }
        else
        {
            L_SDA();
        }
        data <<= 1;
        spi_delay_time();
        L_SCK();
        spi_delay_time();
        H_SCK();
        spi_delay_time();
    }
}

static void spi_write_cmd(unsigned char cmd)
{
    L_CS();
    spi_delay_time();

#if (SPI_LINE_NUM == 3)
    L_SDA();
    spi_delay_time();
    L_SCK();
    spi_delay_time();
    H_SCK();
    spi_delay_time();
#elif (SPI_LINE_NUM == 4)
    L_RS(); /* 0-Command  1-Data */
#endif

    spi_send_byte(cmd);
    spi_delay_time();

    H_CS();
    spi_delay_time();
}

static void spi_write_data(unsigned char data)
{
    L_CS();
    spi_delay_time();

#if (SPI_LINE_NUM == 3)
    H_SDA();
    spi_delay_time();
    L_SCK();
    spi_delay_time();
    H_SCK();
    spi_delay_time();
#elif (SPI_LINE_NUM == 4)
    H_RS(); /* 0-Command  1-Data */
#endif

    spi_send_byte(data);
    spi_delay_time();

    H_CS();
    spi_delay_time();
}

static void spi_port_init(void)
{
#if (SPI_LINE_NUM == 4)
    GPIO_Init(LCD_GPIO_RS, LCD_PIN_RS, 1, 1, 0, 0);
#endif
    GPIO_Init(LCD_GPIO_CS, LCD_PIN_CS, 1, 1, 0, 0);
    GPIO_Init(LCD_GPIO_SCK, LCD_PIN_SCK, 1, 1, 0, 0);
    GPIO_Init(LCD_GPIO_SDA, LCD_PIN_SDA, 1, 1, 0, 0);
    GPIO_Init(LCD_GPIO_RST, LCD_PIN_RST, 1, 1, 0, 0);
    H_CS();
    H_SCK();
    H_SDA();
}

/*******************************************************************************************************************************************
 * Public Function
 *******************************************************************************************************************************************/
/**
 * @brief 通过 SPI 通讯初始化屏幕
 */
void lcd_spi_init_JLT35002A(int hres, int vres)
{
    spi_port_init();

    /* 复位时序, 请按照规格书进行 */
    H_RST();
    systick_delay_ms(1);
    L_RST();
    systick_delay_ms(10);
    H_RST();
    systick_delay_ms(120);

    //函数名替换
    #define lcd_write_index         spi_write_cmd
    #define lcd_write_byte          spi_write_data

    //晶力泰提供
#if 1
    //*************LCD Driver Initial **********//
    lcd_write_index(0xE0);//PGAMCTRL (Positive Gamma Control)
    lcd_write_byte(0x00);
    lcd_write_byte(0x07);
    lcd_write_byte(0x10);
    lcd_write_byte(0x09);
    lcd_write_byte(0x17);
    lcd_write_byte(0x0B);
    lcd_write_byte(0x40);
    lcd_write_byte(0x8A);
    lcd_write_byte(0x4B);
    lcd_write_byte(0x0A);
    lcd_write_byte(0x0D);
    lcd_write_byte(0x0F);
    lcd_write_byte(0x15);
    lcd_write_byte(0x16);
    lcd_write_byte(0x0F);

    lcd_write_index(0xE1);//NGAMCTRL (Negative Gamma Control)
    lcd_write_byte(0x00);
    lcd_write_byte(0x1A);
    lcd_write_byte(0x1B);
    lcd_write_byte(0x02);
    lcd_write_byte(0x0D);
    lcd_write_byte(0x05);
    lcd_write_byte(0x30);
    lcd_write_byte(0x35);
    lcd_write_byte(0x43);
    lcd_write_byte(0x02);
    lcd_write_byte(0x0A);
    lcd_write_byte(0x09);
    lcd_write_byte(0x32);
    lcd_write_byte(0x36);
    lcd_write_byte(0x0F);

    lcd_write_index(0xB1);//Frame Rate Control (In Normal Mode/Full Colors)
    lcd_write_byte(0xB0);
    lcd_write_byte(0x11);

    lcd_write_index(0xB4);//Display Inversion Control
    lcd_write_byte(0x02);

    lcd_write_index(0xC0);//Power Control 1
    lcd_write_byte(0x17);
    lcd_write_byte(0x15);

    lcd_write_index(0xC1);//Power Control 2
    lcd_write_byte(0x41);

    lcd_write_index(0xC5);//VCOM Control
    lcd_write_byte(0x00);
    lcd_write_byte(0x20);
    lcd_write_byte(0x80);

    lcd_write_index(0xB6);//Display Function Control
    lcd_write_byte(0X20);// <important>
    lcd_write_byte(0x22);
#endif    

    lcd_write_index(0x36);//Memory Access Control
	if (hres == 480 && vres == 320) //横屏
	{
		lcd_write_byte(0xF8);// <important>
	}
	else if (hres == 320 && vres == 480) //竖屏
	{
		lcd_write_byte(0x58);// <important> 
	}


    lcd_write_index(0x3a);  //Interface Pixel Format
#if 0
    lcd_write_byte(0x66); /* 0x66 : 18 bits/pixel  */
#else
    lcd_write_byte(0x55); /* 0x55 : 16 bits/pixel  */
#endif

    lcd_write_index(0xE9);//Set Image Function
    lcd_write_byte(0x00);

    lcd_write_index(0XF7);//Adjust Control 3
    lcd_write_byte(0xA9);
    lcd_write_byte(0x51);
    lcd_write_byte(0x2C);
    lcd_write_byte(0x82);//DSI write DCS command, use loose packet RGB 666

	if (hres == 480 && vres == 320) //横屏
	{
		lcd_write_index(0x2A);//Column Address Set
		lcd_write_byte(0x00);
		lcd_write_byte(0x00);
		lcd_write_byte(0x01);
		lcd_write_byte(0xDF);//480

		lcd_write_index(0x2B);//Page Address Set
		lcd_write_byte(0x00);
		lcd_write_byte(0x00);
		lcd_write_byte(0x01);
		lcd_write_byte(0x3F);//320
	}
	else if (hres == 320 && vres == 480) //竖屏
	{
		lcd_write_index(0x2A);//Column Address Set
		lcd_write_byte(0x00);
		lcd_write_byte(0x00);
		lcd_write_byte(0x01);
		lcd_write_byte(0x3F);//320

		lcd_write_index(0x2B);//Page Address Set
		lcd_write_byte(0x00);
		lcd_write_byte(0x00);
		lcd_write_byte(0x01);
		lcd_write_byte(0xDF);//480
	}

    lcd_write_index(0x11);//Sleep OUT

    systick_delay_ms(150);

    lcd_write_index(0x29);//Display ON
}
