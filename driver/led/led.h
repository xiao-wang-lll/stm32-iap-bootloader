#ifndef __LED_H
#define __LED_H

#include "stdint.h"
#include "stdbool.h"


struct led_desc;


typedef struct led_desc *led_desc_t; //结构体数据类型改名





void led_init(led_desc_t led);

void led_set(led_desc_t led,bool onoff);

void led_on(led_desc_t led);

void led_off(led_desc_t led);


#endif
