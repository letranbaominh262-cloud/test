#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "main.h"
#include "cmsis_os.h"

// Port and Pin
#define WDG_PORT		GPIOB
#define WDG_PIN			GPIO_PIN_4
#define ENLS_PORT		GPIOE
#define ENLS_PIN		GPIO_PIN_10
#define EN12_PORT		GPIOE
#define EN12_PIN		GPIO_PIN_5
#define PB1_PERI_PORT	GPIOB
#define PB1_PERI_PIN	GPIO_PIN_1
#define PB2_HUB_PORT	GPIOB
#define PB2_HUB_PIN		GPIO_PIN_2
#define CAN1_STB_PORT	GPIOD
#define CAN1_STB_PIN	GPIO_PIN_2
#define CAN1_SHDN_PORT	GPIOD
#define CAN1_SHDN_PIN	GPIO_PIN_3
#define CAN2_STB_PORT	GPIOD
#define CAN2_STB_PIN	GPIO_PIN_4
#define CAN2_SHDN_PORT	GPIOD
#define CAN2_SHDN_PIN	GPIO_PIN_5
#define RS485_RE_PORT	GPIOA
#define RS485_RE_PIN	GPIO_PIN_0
#define ENRM_PORT		GPIOE
#define ENRM_PIN		GPIO_PIN_2
#define EN_I2C3_PORT    GPIOC
#define EN_I2C3_PIN     GPIO_PIN_6


void app_tasks_init(void);

#endif /* APP_TASKS_H */
