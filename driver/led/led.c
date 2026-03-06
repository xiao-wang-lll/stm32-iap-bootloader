
#include "stm32f4xx.h"
#include "stdbool.h"
#include "board.h"
#include "LED.h"
#include "stdlib.h"
#include "led_desc.h"


void led_init(led_desc_t led){
	
	board_lowlevel_init();
		/* GPIOG Peripheral clock enable */
  
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_StructInit(&GPIO_InitStructure);

  /* Configure PA6 and PA7 in output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = led->GPIO_Pin;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Fast_Speed;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(led->GPIOx, &GPIO_InitStructure);


	//默认两灯都是灭的
	GPIO_SetBits(led->GPIOx,led->GPIO_Pin);
	GPIO_SetBits(led->GPIOx,led->GPIO_Pin);
	
	
}


void led_set(led_desc_t led,bool onoff){
	
	if(led == NULL){
		return;
	}
	
	GPIO_WriteBit(led->GPIOx,led->GPIO_Pin,onoff ? led->OnBit:led->OffBit);

}



void led_on(led_desc_t led){
	
	if(led == NULL){
		return;
	}
		
	//led_set(idx,1);
	GPIO_WriteBit(led->GPIOx,led->GPIO_Pin,led->OnBit);
}

void led_off(led_desc_t led){
	
	if(led == NULL){
		return;
	}
	
	//led_set(idx,0);
	GPIO_WriteBit(led->GPIOx,led->GPIO_Pin,led->OffBit);
}




