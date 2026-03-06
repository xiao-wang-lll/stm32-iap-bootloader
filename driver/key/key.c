
#include "stm32f4xx.h"
#include "stdbool.h"
#include "string.h"
#include "key.h"
#include "key_desc.h"
#include "board.h"



void key_init(key_desc_t key){

		//时钟初始化在board.c里面
		/* GPIOG Peripheral clock enable */
  
	
	
	//GPIO初始化
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_StructInit(&GPIO_InitStructure);

  /* Configure PE3 and PE4 in output pushpull mode */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;
  GPIO_InitStructure.GPIO_PuPd = key->pupd;

	
	GPIO_InitStructure.GPIO_Pin = key->pin ;
  GPIO_Init(key->port, &GPIO_InitStructure);
	
}


bool key_read(key_desc_t key){
	
	
	return  GPIO_ReadInputDataBit(key->port,key->pin) == key->press_level;
	
	

}






