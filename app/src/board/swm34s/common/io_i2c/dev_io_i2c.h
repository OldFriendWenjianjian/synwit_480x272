/**
 *******************************************************************************************************************************************
 * @file		dev_io_i2c.h
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
#ifndef __DEV_IO_I2C_H__
#define __DEV_IO_I2C_H__

#include "SWM341.h"

/**
 * @brief	IO 模拟标准 I2C, 主设备产生起始信号并发送设备地址
 * @param	addr : 传输从设备地址数据
 * @param	wait : 此参数暂无作用, 是为了与硬件 I2C 库函数接口保持一致
 * @retval	1 	 : 接收到 ACK
 * @retval	0 	 : 接收到 NACK
 */
uint8_t io_i2c_start(uint8_t addr, uint8_t wait);

/**
 * @brief	IO 模拟标准 I2C, 主设备读取一个数据
 * @param	ack	 : 1 发送ACK	0 发送NACK
 * @param	wait : 此参数暂无作用, 是为了与硬件 I2C 库函数接口保持一致
 * @retval	读取到的数据
 */
uint8_t io_i2c_read(uint8_t ack, uint8_t wait);

/**
 * @brief	IO 模拟标准 I2C, 主设备写入一个数据
 * @param	data : 要写的数据
 * @param	wait : 此参数暂无作用, 是为了与硬件 I2C 库函数接口保持一致
 * @retval	1 	 : 接收到 ACK
 * @retval	0 	 : 接收到 NACK
 */
uint8_t io_i2c_write(uint8_t data, uint8_t wait);

/**
 * @brief	IO 模拟标准 I2C, 主设备产生停止信号
 * @param	wait : 此参数暂无作用, 是为了与硬件 I2C 库函数接口保持一致
 * @retval	/
 */
void io_i2c_stop(uint8_t wait);

#endif // __DEV_IO_I2C_H__
