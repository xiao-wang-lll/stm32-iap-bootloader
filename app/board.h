#ifndef __BOARD_H
#define __BOARD_H
#include "key.h"
#include "led.h"

void board_lowlevel_init(void);

//两个led板载对象
extern led_desc_t led1;
extern led_desc_t led2;


//两个key板载对象
extern key_desc_t key1;
extern key_desc_t key2;






#endif
