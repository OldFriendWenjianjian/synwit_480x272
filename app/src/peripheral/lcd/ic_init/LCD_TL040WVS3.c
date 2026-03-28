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

static void TL040WVS3_Port_Init(void)
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

void LCD_SPI_Init_TL040WVS3(void)
{
	TL040WVS3_Port_Init();
    
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
    /******************************************************************************/
    /* Panel Name : HSD4.0IPS(HSD040BPN1-A00)                                     */
    /* Resulation : 480x480                                                       */
    /* Inversion  : 2dot                                                          */
    /* Porch      : vbp=15 , vfp=12                                               */
    /* Line Time  : 32us                                                          */
    /* Frame Rate : 60hz                                                          */
    /******************************************************************************/
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
    SPI_WriteData(0x10);
    SPI_WriteComm(0xC0);
    SPI_WriteData(0x3B);
    SPI_WriteData(0x00);
    SPI_WriteComm(0xC1);
    SPI_WriteData(0x0D);
    SPI_WriteData(0x02);
    SPI_WriteComm(0xC2);
    SPI_WriteData(0x21);
    SPI_WriteData(0x08);
    SPI_WriteComm(0xCD);
    SPI_WriteData(0x08);//18-bit/pixel: MDT=0:D[21:16]=R,D[13:8]=G,D[5:0]=B(CDH=00) ;

                     //              MDT=1:D[17:12]=R,D[11:6]=G,D[5:0]=B(CDH=08) ;


    SPI_WriteComm(0xB0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x11);
    SPI_WriteData(0x18);
    SPI_WriteData(0x0E);
    SPI_WriteData(0x11);
    SPI_WriteData(0x06);
    SPI_WriteData(0x07);
    SPI_WriteData(0x08);
    SPI_WriteData(0x07);
    SPI_WriteData(0x22);
    SPI_WriteData(0x04);
    SPI_WriteData(0x12);
    SPI_WriteData(0x0F);
    SPI_WriteData(0xAA);
    SPI_WriteData(0x31);
    SPI_WriteData(0x18);
    SPI_WriteComm(0xB1);
    SPI_WriteData(0x00);
    SPI_WriteData(0x11);
    SPI_WriteData(0x19);
    SPI_WriteData(0x0E);
    SPI_WriteData(0x12);
    SPI_WriteData(0x07);
    SPI_WriteData(0x08);
    SPI_WriteData(0x08);
    SPI_WriteData(0x08);
    SPI_WriteData(0x22);
    SPI_WriteData(0x04);
    SPI_WriteData(0x11);
    SPI_WriteData(0x11);
    SPI_WriteData(0xA9);
    SPI_WriteData(0x32);
    SPI_WriteData(0x18);
    SPI_WriteComm(0xFF);
    SPI_WriteData(0x77);
    SPI_WriteData(0x01);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x11);
    SPI_WriteComm(0xB0);
    SPI_WriteData(0x60);
    SPI_WriteComm(0xB1);
    SPI_WriteData(0x30);
    SPI_WriteComm(0xB2);
    SPI_WriteData(0x87);
    SPI_WriteComm(0xB3);
    SPI_WriteData(0x80);
    SPI_WriteComm(0xB5);
    SPI_WriteData(0x49);
    SPI_WriteComm(0xB7);
    SPI_WriteData(0x85);
    SPI_WriteComm(0xB8);
    SPI_WriteData(0x21);
    SPI_WriteComm(0xC1);
    SPI_WriteData(0x78);
    SPI_WriteComm(0xC2);
    SPI_WriteData(0x78);
    swm_delay_ms(20);
    SPI_WriteComm(0xE0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x1B);
    SPI_WriteData(0x02);
    SPI_WriteComm(0xE1);
    SPI_WriteData(0x08);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x07);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x44);
    SPI_WriteData(0x44);
    SPI_WriteComm(0xE2);
    SPI_WriteData(0x11);
    SPI_WriteData(0x11);
    SPI_WriteData(0x44);
    SPI_WriteData(0x44);
    SPI_WriteData(0xED);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0xEC);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteComm(0xE3);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x11);
    SPI_WriteData(0x11);
    SPI_WriteComm(0xE4);
    SPI_WriteData(0x44);
    SPI_WriteData(0x44);
    SPI_WriteComm(0xE5);
    SPI_WriteData(0x0A);
    SPI_WriteData(0xE9);
    SPI_WriteData(0xD8);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x0C);
    SPI_WriteData(0xEB);
    SPI_WriteData(0xD8);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x0E);
    SPI_WriteData(0xED);
    SPI_WriteData(0xD8);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x10);
    SPI_WriteData(0xEF);
    SPI_WriteData(0xD8);
    SPI_WriteData(0xA0);
    SPI_WriteComm(0xE6);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x11);
    SPI_WriteData(0x11);
    SPI_WriteComm(0xE7);
    SPI_WriteData(0x44);
    SPI_WriteData(0x44);
    SPI_WriteComm(0xE8);
    SPI_WriteData(0x09);
    SPI_WriteData(0xE8);
    SPI_WriteData(0xD8);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x0B);
    SPI_WriteData(0xEA);
    SPI_WriteData(0xD8);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x0D);
    SPI_WriteData(0xEC);
    SPI_WriteData(0xD8);
    SPI_WriteData(0xA0);
    SPI_WriteData(0x0F);
    SPI_WriteData(0xEE);
    SPI_WriteData(0xD8);
    SPI_WriteData(0xA0);
    SPI_WriteComm(0xEB);
    SPI_WriteData(0x02);
    SPI_WriteData(0x00);
    SPI_WriteData(0xE4);
    SPI_WriteData(0xE4);
    SPI_WriteData(0x88);
    SPI_WriteData(0x00);
    SPI_WriteData(0x40);
    SPI_WriteComm(0xEC);
    SPI_WriteData(0x3C);
    SPI_WriteData(0x00);
    SPI_WriteComm(0xED);
    SPI_WriteData(0xAB);
    SPI_WriteData(0x89);
    SPI_WriteData(0x76);
    SPI_WriteData(0x54);
    SPI_WriteData(0x02);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xFF);
    SPI_WriteData(0xFF);
    SPI_WriteData(0x20);
    SPI_WriteData(0x45);
    SPI_WriteData(0x67);
    SPI_WriteData(0x98);
    SPI_WriteData(0xBA);
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
    SPI_WriteData(0x00);
    SPI_WriteComm(0x3A);
    SPI_WriteData(0x66);//55/50=16bit(RGB565);66=18bit(RGB666);77��Ĭ�ϲ�д3AH��=24bit(RGB888)



    SPI_WriteComm(0xFF);
    SPI_WriteData(0x77);
    SPI_WriteData(0x01);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x13);
    SPI_WriteComm(0xE8);
    SPI_WriteData(0x00);
    SPI_WriteData(0x0E);
    SPI_WriteComm(0xFF);
    SPI_WriteData(0x77);
    SPI_WriteData(0x01);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteComm(0x11);
    swm_delay_ms(120);
    SPI_WriteComm(0xFF);
    SPI_WriteData(0x77);
    SPI_WriteData(0x01);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x13);
    SPI_WriteComm(0xE8);
    SPI_WriteData(0x00);
    SPI_WriteData(0x0C);
    swm_delay_ms(10);
    SPI_WriteComm(0xE8);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteComm(0xFF);
    SPI_WriteData(0x77);
    SPI_WriteData(0x01);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);
    SPI_WriteData(0x00);

    SPI_WriteComm(0x2A);
    SPI_WriteComm(0x29);
    swm_delay_ms(20);
#endif
}
