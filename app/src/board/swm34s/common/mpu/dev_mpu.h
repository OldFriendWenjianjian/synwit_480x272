#ifndef __DEV_MPU_H__
#define __DEV_MPU_H__

//----------------------------头文件依赖----------------------------//
#include "main.h"

//----------------------------类型声明----------------------------//
// MPU_LCD 的 COG 初始化
typedef void (*mpu_lcd_init_t)(void);
// MPU_LCD 设置显示区域
typedef void (*mpu_lcd_setcursor_t)(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye);
// MPU_LCD 描点函数
typedef void (*mpu_lcd_drawpoint_t)(uint16_t x, uint16_t y, uint16_t rgb);
// MPU_LCD 全屏单色绘图(纯软件描点,效率低下仅用于测试)
typedef void (*mpu_lcd_clear_t)(uint16_t rgb);
// MPU_LCD 全屏 DMA 绘图(阻塞式, 无中断方式)
typedef void (*mpu_lcd_dma_clear_t)(uint32_t *data);

// MPU 显示方法类
typedef struct {
	mpu_lcd_init_t 			init;
	mpu_lcd_setcursor_t 	setcur;
	mpu_lcd_drawpoint_t 	dp;
	mpu_lcd_clear_t 		clear;
	mpu_lcd_dma_clear_t 	dma_clear;
}mpu_lcd_fun_t;

//----------------------------变量声明----------------------------//
extern volatile mpu_lcd_fun_t MPU_LCD;//实例

#endif //__DEV_MPU_LCD_H__