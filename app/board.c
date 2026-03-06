#include "stm32f4xx.h"
#include "stdio.h"
#include "led.h"
#include "led_desc.h"
#include "key.h"
#include "key_desc.h"
#include "FreeRTOS.h"
#include "task.h"

void board_lowlevel_init(void){
	
	//SCB->VTOR=0x08010000;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);//stm32串口操作esp32
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);//stm32串口调试printf
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);//bootloader
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

}


void board_init(void){
	
	

}

//led的初始化
static struct led_desc _led1={GPIOA,GPIO_Pin_6,Bit_RESET,Bit_SET};
static struct led_desc _led2={GPIOA,GPIO_Pin_7,Bit_RESET,Bit_SET};
led_desc_t led1=&_led1;
led_desc_t led2=&_led2;



//key的初始化
static struct key_desc _key1 = { GPIOE, GPIO_Pin_4, GPIO_PuPd_UP, Bit_RESET};
static struct key_desc _key2 = { GPIOE, GPIO_Pin_3, GPIO_PuPd_UP, Bit_RESET};
key_desc_t key1 = &_key1;
key_desc_t key2 = &_key2;


int fputc(int c,FILE *f){
	
	

	USART_SendData(USART3,(uint8_t)c);
	while(USART_GetFlagStatus(USART3,USART_FLAG_TXE) == RESET);
	USART_ClearFlag(USART3,USART_FLAG_TXE);
	
	return c;


}

void vAssertCalled(const char *file, int line)
{
    printf("Assert Called: %s(%d)\n", file, line);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("Stack Overflowed: %s\n", pcTaskName);
    configASSERT(0);
}

void vApplicationMallocFailedHook(void)
{
    printf("Malloc Failed\n");
    configASSERT(0);
}







