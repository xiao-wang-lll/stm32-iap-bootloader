#ifndef __LED_DESC_H
#define __LED_DESC_H


#include "stm32f4xx.h"

struct led_desc{							//定义LED结构体

	GPIO_TypeDef* GPIOx;
	uint16_t GPIO_Pin;
	BitAction OnBit;
	BitAction OffBit;
};


#endif
