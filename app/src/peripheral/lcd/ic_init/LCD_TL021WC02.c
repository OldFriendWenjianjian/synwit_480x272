//----------------------------头文件依赖----------------------------//
#include "swm34s/common/systick/dev_systick.h"

#if defined(SWM34SME_A1)// 34SMET6_QFN80
#define GPIO_LCDRST 			GPIOC
#define PIN_LCDRST 				PIN7

#define GPIO_LCDCS 				GPIOA
#define PIN_LCDCS 				PIN13

#define GPIO_LCDSCK 			GPIOD
#define PIN_LCDSCK 				PIN1

#define GPIO_LCDSDA 			GPIOD
#define PIN_LCDSDA 				PIN0

#else // 34SVET6_LQFP100
#define GPIO_LCDRST 			GPIOD
#define PIN_LCDRST 				PIN1

#define GPIO_LCDCS 				GPIOB
#define PIN_LCDCS 				PIN0

#define GPIO_LCDSCK 			GPION
#define PIN_LCDSCK 				PIN5

#define GPIO_LCDSDA 			GPION
#define PIN_LCDSDA 				PIN4

#endif

//操作宏
#if 0 //库函数
#define PIN_LCDRST_HIGH() 		GPIO_SetBit(GPIO_LCDRST, PIN_LCDRST)
#define PIN_LCDRST_LOW() 		GPIO_ClrBit(GPIO_LCDRST, PIN_LCDRST)

#define PIN_LCDCS_HIGH() 		GPIO_SetBit(GPIO_LCDCS, PIN_LCDCS)
#define PIN_LCDCS_LOW() 		GPIO_ClrBit(GPIO_LCDCS, PIN_LCDCS)

#define PIN_LCDSCK_HIGH() 		GPIO_SetBit(GPIO_LCDSCK, PIN_LCDSCK)
#define PIN_LCDSCK_LOW() 		GPIO_ClrBit(GPIO_LCDSCK, PIN_LCDSCK)

#define PIN_LCDSDA_HIGH() 		GPIO_SetBit(GPIO_LCDSDA, PIN_LCDSDA)
#define PIN_LCDSDA_LOW() 		GPIO_ClrBit(GPIO_LCDSDA, PIN_LCDSDA)

#else //寄存器
#define PIN_LCDRST_HIGH() 		GPIO_LCDRST->ODR |= (0x01 << PIN_LCDRST)
#define PIN_LCDRST_LOW() 		GPIO_LCDRST->ODR &= ~(0x01 << PIN_LCDRST)

#define PIN_LCDCS_HIGH() 		GPIO_LCDCS->ODR |= (0x01 << PIN_LCDCS)
#define PIN_LCDCS_LOW() 		GPIO_LCDCS->ODR &= ~(0x01 << PIN_LCDCS)

#define PIN_LCDSCK_HIGH() 		GPIO_LCDSCK->ODR |= (0x01 << PIN_LCDSCK)
#define PIN_LCDSCK_LOW() 		GPIO_LCDSCK->ODR &= ~(0x01 << PIN_LCDSCK)

#define PIN_LCDSDA_HIGH() 		GPIO_LCDSDA->ODR |= (0x01 << PIN_LCDSDA)
#define PIN_LCDSDA_LOW() 		GPIO_LCDSDA->ODR &= ~(0x01 << PIN_LCDSDA)

#endif

//----------------------------本地函数定义----------------------------//
static inline void Delay(void)
{
	for (int us = 0; us < 100; us++)
		__NOP();
}

static void SPI_SendData(unsigned char i)
{
	for (unsigned char n = 0; n < 8; n++)
	{
		if (i & 0x80)
		{
			PIN_LCDSDA_HIGH();
		}
		else
		{
			PIN_LCDSDA_LOW();
		}
		i <<= 1;
		Delay();
		PIN_LCDSCK_LOW();
		Delay();
		PIN_LCDSCK_HIGH();
		Delay();
	}
}

static void SPI_WriteComm(unsigned char i)
{
	PIN_LCDCS_LOW();
	Delay();
	PIN_LCDSDA_LOW();
	Delay();
	PIN_LCDSCK_LOW();
	Delay();

	PIN_LCDSCK_HIGH();
	Delay();
	SPI_SendData(i);
	Delay();
	PIN_LCDCS_HIGH();
	Delay();
}

static void SPI_WriteData(unsigned char i)
{
	PIN_LCDCS_LOW();
	Delay();
	PIN_LCDSDA_HIGH();
	Delay();
	PIN_LCDSCK_LOW();
	Delay();

	PIN_LCDSCK_HIGH();
	Delay();
	SPI_SendData(i);
	Delay();
	PIN_LCDCS_HIGH();
	Delay();
}

static void spi_Port_Init(void)
{
	GPIO_Init(GPIO_LCDCS, PIN_LCDCS, 1, 1, 0, 0);
	GPIO_Init(GPIO_LCDSCK, PIN_LCDSCK, 1, 1, 0, 0);
	GPIO_Init(GPIO_LCDSDA, PIN_LCDSDA, 1, 1, 0, 0);
	GPIO_Init(GPIO_LCDRST, PIN_LCDRST, 1, 1, 0, 0);
	PIN_LCDCS_HIGH();
	PIN_LCDSCK_HIGH();
	PIN_LCDSDA_HIGH();

	PIN_LCDRST_HIGH(); //复位
	Delay();//swm_delay_ms(1);
	PIN_LCDRST_LOW();
	swm_delay_ms(1);
	PIN_LCDRST_HIGH();
}

//----------------------------对外函数定义----------------------------//

void LCD_SPI_Init_TL021WC02(void)
{
	spi_Port_Init();

	PIN_LCDCS_LOW();
	Delay();
	PIN_LCDSDA_HIGH();
	swm_delay_ms(8);

	PIN_LCDSDA_LOW();
	swm_delay_ms(50);
	PIN_LCDSDA_HIGH();
	swm_delay_ms(50);

	SPI_WriteComm(0x11); //BOE3.97IPS
	swm_delay_ms(150);

#if 1 //缩进便于观看
    SPI_WriteComm(0xFF);
    SPI_WriteData(0x77);
    SPI_WriteData(0x01);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x10);

    SPI_WriteComm(0xC0);
    SPI_WriteData(0x3B);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xC1);
    SPI_WriteData(0x0B); //VBP
    SPI_WriteData(0x02);

    SPI_WriteComm(0xC2);
    SPI_WriteData(0x00);
    SPI_WriteData(0x02);

    SPI_WriteComm(0xCC);
    SPI_WriteData(0x10);

    SPI_WriteComm(0xCD);
    SPI_WriteData(0x08);

    SPI_WriteComm(0xB0); //Positive Voltage Gamma Control
    SPI_WriteData(0x02);
    SPI_WriteData(0x13);
    SPI_WriteData(0x1B);
    SPI_WriteData(0x0D);
    SPI_WriteData(0x10);
    SPI_WriteData(0x05);
    SPI_WriteData(0x08);
    SPI_WriteData(0x07);
    SPI_WriteData(0x07);
    SPI_WriteData(0x24);
    SPI_WriteData(0x04);
    SPI_WriteData(0x11);
    SPI_WriteData(0x0E);
    SPI_WriteData(0x2C);
    SPI_WriteData(0x33);
    SPI_WriteData(0x1D);

    SPI_WriteComm(0xB1); //Negative Voltage Gamma Control
    SPI_WriteData(0x05);
    SPI_WriteData(0x13);
    SPI_WriteData(0x1B);
    SPI_WriteData(0x0D);
    SPI_WriteData(0x11);
    SPI_WriteData(0x05);
    SPI_WriteData(0x08);
    SPI_WriteData(0x07);
    SPI_WriteData(0x07);
    SPI_WriteData(0x24);
    SPI_WriteData(0x04);
    SPI_WriteData(0x11);
    SPI_WriteData(0x0E);
    SPI_WriteData(0x2C);
    SPI_WriteData(0x33);
    SPI_WriteData(0x1D);

    SPI_WriteComm(0xFF);
    SPI_WriteData(0x77);
    SPI_WriteData(0x01);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x11);

    SPI_WriteComm(0xB0);
    SPI_WriteData(0x5d); //5d

    SPI_WriteComm(0xB1); //VCOM amplitude setting
    SPI_WriteData(0x43); //43

    SPI_WriteComm(0xB2); //VGH Voltage setting
    SPI_WriteData(0x81); //12V

    SPI_WriteComm(0xB3);
    SPI_WriteData(0x80);

    SPI_WriteComm(0xB5); //VGL Voltage setting
    SPI_WriteData(0x43); //-8.3V

    SPI_WriteComm(0xB7);
    SPI_WriteData(0x85);

    SPI_WriteComm(0xB8);
    SPI_WriteData(0x20);

    SPI_WriteComm(0xC1);
    SPI_WriteData(0x78);

    SPI_WriteComm(0xC2);
    SPI_WriteData(0x78);

    SPI_WriteComm(0xD0);
    SPI_WriteData(0x88);

    SPI_WriteComm(0xE0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x02);

    SPI_WriteComm(0xE1);
    SPI_WriteData(0x03);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x04);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x20);
    SPI_WriteData(0x20);

    SPI_WriteComm(0xE2);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xE3);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x11);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xE4);
    SPI_WriteData(0x22);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xE5);
    SPI_WriteData(0x05);
    SPI_WriteData(0xEC);
    SPI_WriteData(0xA0);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x07);
    SPI_WriteData(0xEE);
    SPI_WriteData(0xA0);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xE6);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x11);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xE7);
    SPI_WriteData(0x22);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xE8);
    SPI_WriteData(0x06);
    SPI_WriteData(0xED);
    SPI_WriteData(0xA0);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x08);
    SPI_WriteData(0xEF);
    SPI_WriteData(0xA0);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xEB);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x40);
    SPI_WriteData(0x40);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xED);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xBA);
    SPI_WriteData(0x0A);
    SPI_WriteData(0xBF);
    SPI_WriteData(0x45);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xFF);
    SPI_WriteData(0x54);
    SPI_WriteData(0xFB);
    SPI_WriteData(0xA0);
    SPI_WriteData(0xAB);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xFF);

    SPI_WriteComm(0xEF);
    SPI_WriteData(0x10);
    SPI_WriteData(0x0D);
    SPI_WriteData(0x04);
    SPI_WriteData(0x08);
    SPI_WriteData(0x3F);
    SPI_WriteData(0x1F);

    SPI_WriteComm(0xFF);
    SPI_WriteData(0x77);
    SPI_WriteData(0x01);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x13);

    SPI_WriteComm(0xEF);
    SPI_WriteData(0x08);

    SPI_WriteComm(0xFF);
    SPI_WriteData(0x77);
    SPI_WriteData(0x01);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);

    SPI_WriteComm(0x11);
#endif

    swm_delay_ms(120);

    SPI_WriteComm(0x29);

    SPI_WriteComm(0x36);
    SPI_WriteData(0x00);

    SPI_WriteComm(0x3A);
    SPI_WriteData(0x60);
}
