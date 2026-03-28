//-----------------------------头文件依赖---------------------------//
static uint16_t LCD_HDOT; // 水平点数
static uint16_t LCD_VDOT; // 垂直点数

//----------------------------全局宏----------------------------//
//寄存器命令-未使用该宏,为更清晰直观,在初始化中逐一写入指令
#define ILI9488_CMD_COUSOR_X 	0x2A //0x2A00
#define ILI9488_CMD_COUSOR_Y 	0x2B //0x2B00
#define ILI9488_CMD_WR_DATA 	0x2C //0x2C00

#define HV_Flag 				0 //取值 0 ~ 3 : 设置屏幕显示方向

//----------------------------MPU-LCD 接口函数定义----------------------------//
static void ILI9488_SetDirection(int flag)
{
/**
  * Memory Access Control (36h)
  * This command defines read/write scanning direction of the frame memory.
  *
  * These 3 bits control the direction from the MPU to memory write/read.
  *
  * Bit  Symbol  Name  Description
  * D7   MY  Row Address Order     -- 以X轴镜像
  * D6   MX  Column Address Order  -- 以Y轴镜像
  * D5   MV  Row/Column Exchange   -- X轴与Y轴交换
  * D4   ML  Vertical Refresh Order  LCD vertical refresh direction control. 
  *
  * D3   BGR RGB-BGR Order   Color selector switch control
  *      (0 = RGB color filter panel, 1 = BGR color filter panel )
  * D2   MH  Horizontal Refresh ORDER  LCD horizontal refreshing direction control.
  * D1   X   Reserved  Reserved
  * D0   X   Reserved  Reserved
  */
	if (flag == 0)
	{
		//   左上角->右下角
		//	(0,0)	___ x(320)
		//	     |
		//	     |
		//       |	y(480)
		LCD_WR_REG(LCD,0x36);
		LCD_WR_DATA(LCD,0x48);//JLT35002 V3 write for 0x48

		LCD_WR_REG(LCD,0x2A);
		LCD_WR_DATA(LCD,0x00); /* x start */
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x01); /* x end */
		LCD_WR_DATA(LCD,0x3F);

		LCD_WR_REG(LCD,0x2B);
		LCD_WR_DATA(LCD,0x00); /* y start */
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x01); /* y end */
		LCD_WR_DATA(LCD,0xDF);
	}
	else if (flag == 1)
	{
		//		右上角-> 左下角
		//		y(320)___ (0,0)
		//		         |
		//		         |
		//             |x(480)
		LCD_WR_REG(LCD,0x36);
		LCD_WR_DATA(LCD,0x68);
		LCD_WR_REG(LCD,0x2A);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x01);
		LCD_WR_DATA(LCD,0xDF);

		LCD_WR_REG(LCD,0x2B);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x01);
		LCD_WR_DATA(LCD,0x3F);
	}
	else if (flag == 2)
	{
		//		右下角->左上角
		//		          |y(480)
		//		          |
		//		x(320) ___|(0,0)
		LCD_WR_REG(LCD,0x36);
		LCD_WR_DATA(LCD,0xC8);
		LCD_WR_REG(LCD,0x2A);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x01);
		LCD_WR_DATA(LCD,0x3F);

		LCD_WR_REG(LCD,0x2B);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x01);
		LCD_WR_DATA(LCD,0x3F);
	}
	else if (flag == 3)
	{
		//		左下角->右上角
		//		|x(480)
		//		|
		//		|___ y(320)
		LCD_WR_REG(LCD,0x36);
		LCD_WR_DATA(LCD,0x28);

		LCD_WR_REG(LCD,0x2A);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x01);
		LCD_WR_DATA(LCD,0xDF);

		LCD_WR_REG(LCD,0x2B);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x00);
		LCD_WR_DATA(LCD,0x01);
		LCD_WR_DATA(LCD,0x3F);
	}
}

/****************************************************************************************************************************************** 
* 函数名称:	mpu_lcd_init()
* 功能说明: TFT液晶屏初始化，TFT使用ILI9488驱动，分辨率320*480，
*			屏幕模式：竖屏，从左到右、从上到下
* 输    入: 无
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void ILI9488_Init(void)
{
	uint32_t id[4];
/*
	id = LCD_ReadReg(LCD, 0XDA00) << 16;
	id |= LCD_ReadReg(LCD, 0XDB00) << 8;
	id |= LCD_ReadReg(LCD, 0XDC00);
	if(id != 0x008000) return;

	//LCD_WR_REG(0XD4);
	id[0]=LCD_ReadReg(LCD,0xDA);//读回0X01
	id[1]=LCD_ReadReg(LCD,0xDB);//读回0X53
	id[2]=LCD_ReadReg(LCD,0xDC);//读回0X53
	//id[3]=LCD_RD_DATA();//读回0X53
*/
	/**/
	LCD->MPUIR = 0xD3;
	while (LCD_IsBusy(LCD))
		__NOP();

	id[0] = LCD->MPUDR;
	while (LCD_IsBusy(LCD))
		__NOP();

	id[1] = LCD->MPUDR;
	while (LCD_IsBusy(LCD))
		__NOP();

	id[2] = LCD->MPUDR;
	while (LCD_IsBusy(LCD))
		__NOP();

	id[3] = LCD->MPUDR;
	while (LCD_IsBusy(LCD))
		__NOP();
	
	//printf("\r\nMPU_LCD ID = 0x[%x%x%x%x]\r\n", id[0], id[1], id[2], id[3]);
	
#if 	1//为了观看时更好地缩进而使用，不可取消置 0
	//************* Start Initial Sequence **********//
	/* PGAMCTRL (Positive Gamma Control) (E0h) */
	LCD_WR_REG(LCD,0xE0);
	LCD_WR_DATA(LCD,0x00);
	LCD_WR_DATA(LCD,0x07);
	LCD_WR_DATA(LCD,0x10);
	LCD_WR_DATA(LCD,0x09);
	LCD_WR_DATA(LCD,0x17);
	LCD_WR_DATA(LCD,0x0B);
	LCD_WR_DATA(LCD,0x41);
	LCD_WR_DATA(LCD,0x89);
	LCD_WR_DATA(LCD,0x4B);
	LCD_WR_DATA(LCD,0x0A);
	LCD_WR_DATA(LCD,0x0C);
	LCD_WR_DATA(LCD,0x0E);
	LCD_WR_DATA(LCD,0x18);
	LCD_WR_DATA(LCD,0x1B);
	LCD_WR_DATA(LCD,0x0F);

	/* NGAMCTRL (Negative Gamma Control) (E1h)  */
	LCD_WR_REG(LCD,0XE1);
	LCD_WR_DATA(LCD,0x00);
	LCD_WR_DATA(LCD,0x17);
	LCD_WR_DATA(LCD,0x1A);
	LCD_WR_DATA(LCD,0x04);
	LCD_WR_DATA(LCD,0x0E);
	LCD_WR_DATA(LCD,0x06);
	LCD_WR_DATA(LCD,0x2F);
	LCD_WR_DATA(LCD,0x45);
	LCD_WR_DATA(LCD,0x43);
	LCD_WR_DATA(LCD,0x02);
	LCD_WR_DATA(LCD,0x0A);
	LCD_WR_DATA(LCD,0x09);
	LCD_WR_DATA(LCD,0x32);
	LCD_WR_DATA(LCD,0x36);
	LCD_WR_DATA(LCD,0x0F);

	/* Adjust Control 3 (F7h)  */
	LCD_WR_REG(LCD,0XF7);
	LCD_WR_DATA(LCD,0xA9);
	LCD_WR_DATA(LCD,0x51);
	LCD_WR_DATA(LCD,0x2C);
	LCD_WR_DATA(LCD,0x82); /* DSI write DCS command, use loose packet RGB 666 */

	/* Power Control 1 (C0h)  */
	LCD_WR_REG(LCD,0xC0);
	LCD_WR_DATA(LCD,0x11);
	LCD_WR_DATA(LCD,0x09);

	/* Power Control 2 (C1h) */
	LCD_WR_REG(LCD,0xC1);
	LCD_WR_DATA(LCD,0x41);

	/* VCOM Control (C5h)  */
	LCD_WR_REG(LCD,0XC5);
	LCD_WR_DATA(LCD,0x00);
	LCD_WR_DATA(LCD,0x0A);
	LCD_WR_DATA(LCD,0x80);

	/* Frame Rate Control (In Normal Mode/Full Colors) (B1h) */
	LCD_WR_REG(LCD,0xB1);
	LCD_WR_DATA(LCD,0xB0);
	LCD_WR_DATA(LCD,0x11);

	/* Display Inversion Control (B4h) */
	LCD_WR_REG(LCD,0xB4);
	LCD_WR_DATA(LCD,0x02);

	/* Display Function Control (B6h)  */
	LCD_WR_REG(LCD,0xB6);
	LCD_WR_DATA(LCD,0x02);
	LCD_WR_DATA(LCD,0x22);

	/* Entry Mode Set (B7h)  */
	LCD_WR_REG(LCD,0xB7);
	LCD_WR_DATA(LCD,0xc6);

	/* HS Lanes Control (BEh) */
	LCD_WR_REG(LCD,0xBE);
	LCD_WR_DATA(LCD,0x00);
	LCD_WR_DATA(LCD,0x04);

	/* Set Image Function (E9h)  */
	LCD_WR_REG(LCD,0xE9);
	LCD_WR_DATA(LCD,0x00);
#endif

	/* 设置屏幕方向和尺寸 */
	ILI9488_SetDirection(HV_Flag);
	
	/* 开始向GRAM写入数据 */
	LCD_WR_REG(LCD,0x2C);	
	
	/* Interface Pixel Format (3Ah) */
	LCD_WR_REG(LCD,0x3A);
	LCD_WR_DATA(LCD,0x55); /* 0x55 : 16 bits/pixel  */

	/* Sleep Out (11h) */
	LCD_WR_REG(LCD,0x11);
	for (uint32_t i = 0; i < 120 * 60000; i++)
		;
	/* Display On */
	LCD_WR_REG(LCD,0x29);
}

/****************************************************************************************************************************************** 
* 函数名称: mpu_lcd_setcursor()
* 功能说明: 
* 输    入: 
* 输    出: 
* 注意事项: 
******************************************************************************************************************************************/
void ILI9488_SetCursor(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye)
{
	LCD_WR_REG(LCD, 0x2A);
	LCD_WR_DATA(LCD, xs >> 8);//xs
	LCD_WR_DATA(LCD, xs & 0xFF);
	LCD_WR_DATA(LCD, xe >> 8);//xe
	LCD_WR_DATA(LCD, xe & 0xFF);

	LCD_WR_REG(LCD, 0x2B);
	LCD_WR_DATA(LCD, ys >> 8);//ys
	LCD_WR_DATA(LCD, ys & 0xFF);
	LCD_WR_DATA(LCD, ye >> 8);//ye
	LCD_WR_DATA(LCD, ye & 0xFF);

	LCD_WR_REG(LCD, 0x2C); //display on
}

/****************************************************************************************************************************************** 
* 函数名称: mpu_lcd_drawpoint()
* 功能说明: 
* 输    入: 
* 输    出: 
* 注意事项: 
******************************************************************************************************************************************/
void ILI9488_DrawPoint(uint16_t x, uint16_t y, uint16_t rgb)
{
	ILI9488_SetCursor(x, x, y, y);

	LCD_WR_DATA(LCD, rgb);
}

/****************************************************************************************************************************************** 
* 函数名称: mpu_lcd_clear()
* 功能说明: 
* 输    入: 
* 输    出: 
* 注意事项: 
******************************************************************************************************************************************/
void ILI9488_Clear(uint16_t rgb)
{
	uint32_t i = 0, j = 0;

	ILI9488_SetCursor(0, LCD_HDOT, 0, LCD_VDOT);
	
	for (i = 0; i < LCD_VDOT; i++)
	{
		for (j = 0; j < LCD_HDOT; j++)
		{
			LCD_WR_DATA(LCD, rgb);
		}
	}
}

/****************************************************************************************************************************************** 
* 函数名称: mpu_lcd_dmaclear()
* 功能说明: 
* 输    入: 
* 输    出: 
* 注意事项: 
******************************************************************************************************************************************/
void ILI9488_DMA_Clear(uint32_t *data)
{
	ILI9488_SetCursor(0, LCD_HDOT, 0, LCD_VDOT);

	MPULCD_DMAStart(LCD, data, LCD_HDOT, LCD_VDOT);

	while (MPULCD_DMABusy(LCD))
		__NOP();
}
