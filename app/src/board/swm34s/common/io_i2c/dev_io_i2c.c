/**
 *******************************************************************************************************************************************
 * @file		dev_io_i2c.c
 * @brief 		IO 模拟 IIC 通讯接口
 * @author   	rk
 * @version 	v1.0.0
 * @date 		2022.12.08
 * @since		v1.0.0 - 2022.12.08
 *******************************************************************************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS WITH CODING INFORMATION
 * REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME. AS A RESULT, SYNWIT SHALL NOT BE HELD LIABLE
 * FOR ANY DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION CONTAINED HEREIN IN CONN-
 * -ECTION WITH THEIR PRODUCTS.
 *
 * @copyright	2012 Synwit Technology
 *******************************************************************************************************************************************
 */
#include "dev_io_i2c.h"
#include "dev_io.h"

//==================================用户适配===================================//
// IO 模拟 I2C 硬件端口

#define IO_I2C_PORT_SDA			TP_GPIO_SDA	
#define IO_I2C_PIN_SDA			TP_PIN_SDA

#define IO_I2C_PORT_SCL			TP_GPIO_SCL	
#define IO_I2C_PIN_SCL			TP_PIN_SCL

//==================================本地私有===================================//
/// 错误重试次数上限
#define MAX_RETRY 			3

/// 本地操作宏
#define IO_I2C_SDA_L() 					IO_SET_L(IO_I2C_PORT_SDA, IO_I2C_PIN_SDA)
#define IO_I2C_SDA_H() 					IO_SET_H(IO_I2C_PORT_SDA, IO_I2C_PIN_SDA)

#define IO_I2C_SCL_L() 					IO_SET_L(IO_I2C_PORT_SCL, IO_I2C_PIN_SCL)
#define IO_I2C_SCL_H() 					IO_SET_H(IO_I2C_PORT_SCL, IO_I2C_PIN_SCL)

#define IO_I2C_SDA_DIR_I()				IO_DIR_I(IO_I2C_PORT_SDA, IO_I2C_PIN_SDA)
#define IO_I2C_SDA_DIR_O()				IO_DIR_O(IO_I2C_PORT_SDA, IO_I2C_PIN_SDA)

//==================================函数声明===================================//
static void io_i2c_clock(void);
static uint8_t io_i2c_check_ack(void);

static uint8_t io_i2c_start_signal(void);
static uint8_t io_i2c_recv_byte(void);
static uint8_t io_i2c_send_byte(uint8_t data);

/**
 * @brief	IIC 通讯的延时保持
 * @note 	因编译优化等级可能会调整, 延时必须充分
 */
static void io_i2c_delay(void)
{
	//#include "dev_systick.h"
	//swm_delay_us(10);
	for (uint32_t i = 0; i < CyclesPerUs; ++i) __NOP();
}

//==================================接口定义===================================//
/**
 * @brief	IO 模拟标准 I2C, 主设备产生起始信号并发送设备地址
 * @param	addr : 传输从设备地址数据
 * @param	wait : 此参数暂无作用, 是为了与硬件 I2C 库函数接口保持一致
 * @retval	1 	 : 接收到 ACK
 * @retval	0 	 : 接收到 NACK
 */
uint8_t io_i2c_start(uint8_t addr, uint8_t wait)
{
	for (uint8_t i = 0; i < MAX_RETRY; ++i)
	{
		if (io_i2c_start_signal())
		{
			if (io_i2c_send_byte(addr))
				return 1;
		}
		io_i2c_stop(1);
	}
	return 0;
}

/**
 * @brief	IO 模拟标准 I2C, 主设备读取一个数据
 * @param	ack	 : 1 发送ACK   0 发送NACK
 * @param	wait : 此参数暂无作用, 是为了与硬件 I2C 库函数接口保持一致
 * @retval	读取到的数据
 */
uint8_t io_i2c_read(uint8_t ack, uint8_t wait)
{
	uint8_t data = 0;

	data = io_i2c_recv_byte();
	IO_I2C_SDA_DIR_O();
	if (ack)
	{
		io_i2c_delay();
		IO_I2C_SDA_L();
		io_i2c_delay();
		io_i2c_clock();
		IO_I2C_SDA_H();
	}
	else
	{
		IO_I2C_SDA_H();
		io_i2c_delay();
		io_i2c_clock();
	}
	return data;
}

/**
 * @brief	IO 模拟标准 I2C, 主设备写入一个数据
 * @param	data : 要写的数据
 * @param	wait : 此参数暂无作用, 是为了与硬件 I2C 库函数接口保持一致
 * @retval	1 	 : 接收到 ACK
 * @retval	0 	 : 接收到 NACK
 */
uint8_t io_i2c_write(uint8_t data, uint8_t wait)
{
	return (io_i2c_send_byte(data)) ? 1 : 0;
}

/**
 * @brief	IO 模拟标准 I2C, 主设备产生停止信号
 * @param	wait : 此参数暂无作用, 是为了与硬件 I2C 库函数接口保持一致
 * @retval	/
 */
void io_i2c_stop(uint8_t wait)
{
	io_i2c_delay();
	IO_I2C_SCL_L();

	IO_I2C_SDA_L();
	io_i2c_delay();
	IO_I2C_SCL_H();
	io_i2c_delay();
	IO_I2C_SDA_H();
}

//==================================私有定义===================================//
//-------------------------------------------------------------------
//函数名称：io_i2c_clock()
//功能说明：I2C总线的SCL从高到低转变的时序，独立模块处理便于调用
//输    出：
//功能说明：
//-------------------------------------------------------------------
static void io_i2c_clock(void)
{
	IO_I2C_SCL_H();
	io_i2c_delay();
	
	// SDA INV 

	IO_I2C_SCL_L();
	io_i2c_delay();
}

//-------------------------------------------------------------------
//函数名称：uint8_t IOi2cStart()
//功能说明：I2C总线启动判断，判断SDA端口，
//输    出：如果为高电平则总线空闲，发起总线起始时序，同时返回1
//          如果为低电平则总线忙，返回0
//注意事项：
//-------------------------------------------------------------------
static uint8_t io_i2c_start_signal(void)
{
	IO_I2C_SDA_H();
	io_i2c_delay();
	IO_I2C_SCL_H();
	io_i2c_delay();

	IO_I2C_SDA_DIR_I(); //切换 SDA 为输入状态

	if (IO_GET(TP_GPIO_SDA, TP_PIN_SDA)) //此处检测 I2C 总线的 IIC_SDA 是否为高电平
	{
		IO_I2C_SDA_DIR_O(); //检测为高电平表明 I2C 总线是空闲的, 则设置 IIC_SDA 为输出状态
		
		//启动 I2C 总线的起始时序
		IO_I2C_SCL_H();
		io_i2c_delay();
		IO_I2C_SDA_H();
		io_i2c_delay();

		IO_I2C_SDA_L();
		io_i2c_delay();
		io_i2c_delay();
		IO_I2C_SCL_L();
		return 1;
	}
	return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//函数名称：io_i2c_receive()
//功能说明：I2C主设备接收8bit数据
//输    出：返回读取的数据值
//功能说明：
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static uint8_t io_i2c_recv_byte(void)
{
	uint8_t data = 0;
	
	IO_I2C_SDA_DIR_I();
	
	for (uint8_t i = 0; i < 8; ++i)
	{
		data <<= 1;
		
		IO_I2C_SCL_H();
		io_i2c_delay();

		if (IO_GET(TP_GPIO_SDA, TP_PIN_SDA)) // MSB 先行
			data++;//data |= (1 << (7 - i));
		
		IO_I2C_SCL_L();
		io_i2c_delay();
	}
	
	IO_I2C_SDA_DIR_O();
	return data;
}

//-------------------------------------------------------------------
//函数名称：uint8_t io_i2c_ack()
//功能说明：
//输    出：ACK为低电平，即有应答，返回1； ACK为高电平，无应答，返回0
//功能说明：
//-------------------------------------------------------------------
static uint8_t io_i2c_check_ack(void)
{
	uint8_t ack = 0;
	
	IO_I2C_SDA_DIR_I();
	io_i2c_delay();
	
	io_i2c_delay();
	IO_I2C_SCL_H();
	io_i2c_delay();
	
	ack = IO_GET(TP_GPIO_SDA, TP_PIN_SDA);
	io_i2c_delay();
	if (ack)
	{
		IO_I2C_SDA_DIR_O();
		io_i2c_delay();
		
		IO_I2C_SCL_L();
		io_i2c_delay();
		
		IO_I2C_SDA_H();
		io_i2c_delay();
	}
	else 
	{
		IO_I2C_SCL_L();
		IO_I2C_SDA_DIR_O();
		io_i2c_delay();
		
		IO_I2C_SDA_H();
		io_i2c_delay();
	}
	return !ack;
}

//-------------------------------------------------------------------
//函数名称：io_i2c_btransmit()
//功能说明：I2C 主设备传输8位数据
//输    出：检测到 ACK 返回1；无ACK返回0
//功能说明：
//-------------------------------------------------------------------
static uint8_t io_i2c_send_byte(uint8_t data)
{
	for (uint8_t i = 0; i < 8; ++i)
	{
//		if ((data >> (7 - i)) & 0x01) // MSB 先行
//			IO_I2C_SDA_H();
//		else
//			IO_I2C_SDA_L();
		
		if( (data & 0x80) == 0x80 )
			IO_I2C_SDA_H();
		else
			IO_I2C_SDA_L();
		data = data << 1;
		
		io_i2c_delay();
		
		io_i2c_clock();
	}
	return io_i2c_check_ack();
}
