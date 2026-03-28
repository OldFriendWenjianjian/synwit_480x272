/**
 *******************************************************************************************************************************************
 * @file		dev_io.h
 * @brief 		IO 模拟外设功能底层操作宏, 用于对接[外设驱动层], [用户层]应使用[驱动层]的接口, 不宜直接使用本模块
 * @author   	lzh
 * @version 	v1.0.0
 * @date 		2022.12.08
 * @since		v1.0.0 - 2022.12.08
 * 
 * @remark		以下操作宏均须在被调用前保证对应 IO 已被初始化为 GPIO 功能, 此项由[驱动层]负责保证
 * 即 < GPIO_Init(port, pin, dir, 1, 0, 0) >
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
#ifndef __DEV_IO_H__
#define __DEV_IO_H__

#include "SWM341.h"

/// 设置 IO 口方向 - 输入
#define IO_DIR_I(GPIOx, PINx)			( ( (GPIOx)->DIR ) &= ~( 0x01 << (PINx) ) )
/// 设置 IO 口方向 - 输出
#define IO_DIR_O(GPIOx, PINx)			( ( (GPIOx)->DIR ) |= ( 0x01 << (PINx) ) )

/// 获取 IO 输入(须保证对应 IO 为 GPIO 输入状态, 否则操作无效)
#define IO_GET(GPIOx, PINx)          	( ( ( (GPIOx)->IDR ) >> (PINx) ) & 0x01 )

/// IO 置低(须保证对应 IO 为 GPIO 输出状态, 否则操作无效)
#define IO_SET_L(GPIOx, PINx)           ( *( ( &( (GPIOx)->DATAPIN0 ) ) + (PINx) ) = 0 )
/// IO 置高(须保证对应 IO 为 GPIO 输出状态, 否则操作无效)
#define IO_SET_H(GPIOx, PINx)           ( *( ( &( (GPIOx)->DATAPIN0 ) ) + (PINx) ) = 1 )

#endif // __DEV_IO_H__
