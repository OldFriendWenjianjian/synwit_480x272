#ifndef __DEV_TP_H__
#define __DEV_TP_H__

#define TP_I2C_INDEX        I2C1

#define TP_GPIO_RST 		GPIOB
#define TP_PIN_RST 			PIN11

#define TP_GPIO_INT 		GPIOD
#define TP_PIN_INT 			PIN0
#define TP_IRQ              GPIOD_IRQn

#define TP_PORT_SCL 		PORTC
#define TP_I2C_SCL_PIN      PIN5
#define TP_I2C_SCL_PIN_FUNC PORTA_PIN1_I2C0_SCL

#define TP_PORT_SDA 		PORTC
#define TP_I2C_SDA_PIN      PIN4
#define TP_I2C_SDA_PIN_FUNC PORTA_PIN0_I2C0_SDA

#endif //__DEV_TP_H__