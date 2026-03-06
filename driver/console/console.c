
#include "stm32f4xx.h"
#include "stdbool.h"
#include "string.h"
#include "console.h"



console_received_func_t console_func;

void console_init(void){

		USART_InitTypeDef USART_InitStructure;
		USART_StructInit(&USART_InitStructure);
	
		USART_InitStructure.USART_BaudRate = 115200;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

			/* Connect PXx to USARTx_Tx*/
		GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);

		/* Connect PXx to USARTx_Rx*/
		GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);
		
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_StructInit(&GPIO_InitStructure);
		
		/* Configure USART Tx as alternate function  */
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		/* Configure USART Rx as alternate function  */
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
		//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
		
		/* USART configuration */
		USART_Init(USART2, &USART_InitStructure);
		
		
		USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
		
			/* Enable USART */
		USART_Cmd(USART2, ENABLE);
		
		//NVIC≥ı ºªØ
		NVIC_InitTypeDef NVIC_InitStructure;
		memset(&NVIC_InitStructure,0,sizeof(NVIC_InitStructure));
		
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
		
		NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=5;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
		NVIC_InitStructure.NVIC_IRQChannel=USART2_IRQn;
		NVIC_Init(&NVIC_InitStructure);

}

void console_write(const char c[],uint32_t size){
	
	
		int temp=size;
		
		for(int i=0;i<temp;i++){
			
			USART_ClearFlag(USART2,USART_FLAG_TC);
			USART_SendData(USART2,(uint16_t)c[i]);
			while(USART_GetFlagStatus(USART2,USART_FLAG_TC) == RESET);
		}
	
}

void console_received_register(console_received_func_t func){

	console_func=func;


}

void USART2_IRQHandler(void){
	
	uint8_t data=0x00;
	
	if(USART_GetFlagStatus(USART2,USART_FLAG_RXNE) != RESET){
		
		data=USART_ReceiveData(USART2);
		USART_ClearFlag(USART2,USART_FLAG_RXNE);
		
		
		if(console_func !=NULL){
		  console_func(data);
	
		}else{
				
			USART_SendData(USART2,0x0000);
		}
	}

}




