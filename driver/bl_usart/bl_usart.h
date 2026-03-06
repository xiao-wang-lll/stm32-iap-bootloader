#ifndef __BL_USART_H
#define __BL_USART_H

#include "stdint.h"

typedef void (*bl_usart_received_func_t)(const uint8_t *data, uint32_t size);


void bl_usart_init(void);

void bl_usart_write(const uint8_t *data, uint32_t size);

void bl_usart_received_register(bl_usart_received_func_t func);


#endif/**BL_USART*/
