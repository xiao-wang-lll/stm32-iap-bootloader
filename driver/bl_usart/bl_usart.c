
#include "stm32f4xx.h"
#include "stdbool.h"
#include "string.h"
#include "bl_usart.h"
//#include "freertos.h"
//#include "semphr.h"

//USART3
//tx  PC10
//rx  PC11
#define BL_DMA_RX_BUF_SIZE 5120 // 必须大于最大单包长度---即大于MTU！
static uint8_t dma_rx_buf[BL_DMA_RX_BUF_SIZE];
static uint16_t last_rx_pos = 0; // 记录上次读取的位置



bl_usart_received_func_t bl_usart_func;



static void bl_usart_io_init(void)
{
    /* Connect PXx to USARTx_Tx*/
		GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_USART3);

		/* Connect PXx to USARTx_Rx*/
		GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_USART3);
		
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_StructInit(&GPIO_InitStructure);
		
		/* Configure USART Tx as alternate function  */
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_Init(GPIOC, &GPIO_InitStructure);

		/* Configure USART Rx as alternate function  */
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
		//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
		GPIO_Init(GPIOC, &GPIO_InitStructure);
}

static void bl_usart_usart_init(void)
{
    	USART_InitTypeDef USART_InitStructure;
		USART_StructInit(&USART_InitStructure);
	
		USART_InitStructure.USART_BaudRate = 115200;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		/* USART configuration */
		USART_Init(USART3, &USART_InitStructure);
		


		// 【关键修改 1】关闭逐字节中断 (RXNE)，开启空闲中断 (IDLE)
		//USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); 
		USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);
		// 【关键修改 2】开启 RX 的 DMA 请求
		USART_DMACmd(USART3,USART_DMAReq_Rx,ENABLE);
			/* Enable USART */
		USART_Cmd(USART3, ENABLE);

		
    
    
    
}

static void bl_usart_dma_init(void)
{
    DMA_InitTypeDef DMA_InitStruct;
    DMA_StructInit(&DMA_InitStruct);
    DMA_InitStruct.DMA_Channel = DMA_Channel_4;
	DMA_InitStruct.DMA_Memory0BaseAddr=(uint32_t)dma_rx_buf;// 指向创建的缓存
	DMA_InitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;// 内存地址自增
	DMA_InitStruct.DMA_BufferSize = BL_DMA_RX_BUF_SIZE;
    DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR;
    DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStruct.DMA_Mode = DMA_Mode_Circular;// 【必须是循环模式】
    DMA_InitStruct.DMA_Priority = DMA_Priority_High;// 接收优先级调高
    DMA_InitStruct.DMA_FIFOMode = DMA_FIFOMode_Disable;// 接收通常关 FIFO
    DMA_InitStruct.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStruct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA1_Stream1, &DMA_InitStruct);
	
	//DMA开启中断
	//DMA_ITConfig(DMA1_Stream3,DMA_IT_TC,ENABLE);
	// 接收 DMA 不需要开传输完成中断，全靠 USART 的 IDLE 中断来处理
    DMA_Cmd(DMA1_Stream1, ENABLE); // 直接启动接收
}

static void bl_usart_int_init(void)
{
    //NVIC初始化
	NVIC_InitTypeDef NVIC_InitStructure;
	memset(&NVIC_InitStructure,0,sizeof(NVIC_InitStructure));
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=5;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
	NVIC_InitStructure.NVIC_IRQChannel=USART3_IRQn; 

	NVIC_Init(&NVIC_InitStructure);
	
	//USART3的tx---这些关掉主要使用，USART的空闲中断
	// NVIC_InitStructure.NVIC_IRQChannel=DMA1_Stream3_IRQn;
	// NVIC_Init(&NVIC_InitStructure);
	//USART3的rx
	NVIC_InitStructure.NVIC_IRQChannel=DMA1_Stream1_IRQn;
	NVIC_Init(&NVIC_InitStructure);

}

void bl_usart_init(void)
{

	
    bl_usart_usart_init();
    
	bl_usart_dma_init();
    bl_usart_int_init();
	bl_usart_io_init();
    
}


void bl_usart_write(const uint8_t *data, uint32_t size){
	
	
	while (size--)
	{
		while(USART_GetFlagStatus(USART3,USART_FLAG_TXE) == RESET);
		USART_SendData(USART3,*data++);
	}

	//保证发送数据都发完了
	while(USART_GetFlagStatus(USART3,USART_FLAG_TC) == RESET);
	USART_ClearFlag(USART3, USART_FLAG_TC);
	
}


void bl_usart_received_register(bl_usart_received_func_t func){

	bl_usart_func=func;


}

void USART3_IRQHandler(void){
	
	
	if(USART_GetFlagStatus(USART3,USART_FLAG_IDLE) != RESET){
		
		// 1. 清除 IDLE 标志位 (必须按顺序：先读 SR，再读 DR)
        volatile uint32_t temp = USART3->SR;
        temp = USART3->DR;

		// 2. 计算当前 DMA 写到哪个位置了
        // NDTR 寄存器存的是“还剩多少没搬”，用总长度减去它，就是当前位置
        uint16_t curr_pos = BL_DMA_RX_BUF_SIZE - DMA_GetCurrDataCounter(DMA1_Stream1);

        // 3. 把新收到的数据打包发给 main.c 的 RingBuffer
        if (bl_usart_func != NULL) 
        {
            if (curr_pos > last_rx_pos) 
            {
                // 正常情况：指针一直往后走
                // 一次性把这批数据传过去 (长度 = curr_pos - last_rx_pos)
                bl_usart_func(&dma_rx_buf[last_rx_pos], curr_pos - last_rx_pos);
            } 
            else if (curr_pos < last_rx_pos) 
            {
                // 回卷情况：DMA 跑到末尾后又从头开始了 (因为是 Circular 模式)
                // 第一段：先把末尾剩下的部分传过去
                bl_usart_func(&dma_rx_buf[last_rx_pos], BL_DMA_RX_BUF_SIZE - last_rx_pos);
                // 第二段：再把头部的新数据传过去
                if (curr_pos > 0) {
                    bl_usart_func(&dma_rx_buf[0], curr_pos);
                }
            }
        }

        // 4. 更新指针，留给下一次中断使用
        last_rx_pos = curr_pos;
    }
}




