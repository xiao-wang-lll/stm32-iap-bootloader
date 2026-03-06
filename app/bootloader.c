#include <stdint.h>
#include "stm32f4xx.h"
#include "bl_usart.h"
#include "stdbool.h"
#include "stdio.h"
#include "ringbuffer.h"
#include "crc16.h"
#include "crc32.h"
#include "tim_delay.h"
#include "string.h"
#include "stm32_flash.h"
#include "board.h"
#include "magic_header.h"

#define LOG_TAG "boot"
#define LOG_LVL ELOG_LVL_INFO
#include "elog.h"




#define BL_VERSION      "0.0.1"
#define BL_ADDRESS      0x08000000
#define BL_SIZE         (48 * 1024)
#define APP_VTOR_ADDR   0x08010000
#define BOOT_DELAY      3000
#define RX_BUFFER_SIZE  (5 * 1024)
#define RX_TIMEOUT_MS   20
#define PAYLOAD_SIZE_MAX (4096 + 8) // 4096为Program最大数据长度，8为Program指令的地址(4)和长度(4)
#define PACKET_SIZE_MAX (4 + PAYLOAD_SIZE_MAX + 2) // header(1) + opcode(1) + length(2) + payload + crc16(2)
#define third_wait_boot 1


typedef enum
{
    PACKET_STATE_HEADER,
    PACKET_STATE_OPCODE,
    PACKET_STATE_LENGTH,
    PACKET_STATE_PAYLOAD,
    PACKET_STATE_CRC16,
} packet_state_machine_t;

typedef enum
{
    PACKET_OPCODE_INQUERY = 0x01,
    PACKET_OPCODE_ERASE = 0x81,
    PACKET_OPCODE_PROGRAM = 0x82,
    PACKET_OPCODE_VERIFY  = 0x83,
    PACKET_OPCODE_RESET = 0x21,
    PACKET_OPCODE_BOOT = 0x22,
} packet_opcode_t;

typedef enum
{
    INQUERY_SUBCODE_VERSION = 0x00,
    INQUERY_SUBCODE_MTU = 0x01,
} inquery_subcode_t;


typedef enum
{
    PACKET_ERRCODE_OK = 0,
    PACKET_ERRCODE_OPCODE,
    PACKET_ERRCODE_OVERFLOW,
    PACKET_ERRCODE_TIMEOUT,
    PACKET_ERRCODE_FORMAT,
    PACKET_ERRCODE_VERIFY,
    PACKET_ERRCODE_PARAM,
    PACKET_ERRCODE_UNKNOWN = 0xff,
} packet_errcode_t;

//定义一个rb_buffer数组去传入ringbuffer的new建立的时候给的length初始限制长度
static uint8_t rb_buffer[RX_BUFFER_SIZE];
static rb_t rxrb;


static uint8_t packet_buffer[PACKET_SIZE_MAX];
static uint32_t packet_index;
static packet_state_machine_t packet_state = PACKET_STATE_HEADER;
static packet_opcode_t packet_opcode;
static uint16_t packet_payload_length;


//程序验证
static bool application_validate(void)
{
    if (!magic_header_validate())
    {
        log_e("magic header invalid");
        return false;
    }
    
    //固件校验
    uint32_t addr = magic_header_get_address();
    uint32_t size = magic_header_get_length();
    uint32_t crc = magic_header_get_crc32();//这个crc是固件的校验
    uint32_t ccrc = crc32((uint8_t *)addr, size);
    if (crc != ccrc)
    {
        log_e("application crc error: expected %08X, got %08X", crc, ccrc);
        return false;
    }

    return true;
}

//boot启动
static void boot_application(void)
{
    if (!application_validate())
    {
        log_e("application validate failed, cannot boot");
        return;
    }

    log_i("booting application...");
    tim_delay_ms(2);

    led_off(led1);
    TIM_DeInit(TIM6);
    USART_DeInit(USART3);
    NVIC_DisableIRQ(TIM6_DAC_IRQn);
    NVIC_DisableIRQ(USART3_IRQn);

    //SCB->VTOR=APP_VTOR_ADDR;
    extern void JumpApp(uint32_t base);
    JumpApp(APP_VTOR_ADDR);
}


//响应函数，其返回的值 要对应tlv协议文档的响应格式
static void bl_response(packet_opcode_t opcode,packet_errcode_t errcode,uint8_t *data,uint32_t length){

    //开始执行
    uint8_t *response = packet_buffer;
    response[0] = 0x55;
    response[1] = opcode;
    response[2] = errcode;//这里改成先发送error_code
    response[3] = (uint8_t)(length & 0xFF);//长度,这里采用小端发送
    response[4] = (uint8_t)((length >> 8) & 0xFF);
    if (length > 0) memcpy(&response[5], data, length);

    uint16_t crc = crc16(response, 5 + length);
    response[5 + length] = (uint8_t)(crc & 0xFF);//CRC校验码,这里采用小端发送,并且注意这里发送的是响应数据包的校验码，和之前上位机传过来的数据包的校验码有区别
    response[6 + length] = (uint8_t)((crc >> 8) & 0xFF);

    bl_usart_write(response, 7 + length);


}



//去对上位机发过来的查询指令  进行 response
static void bl_opcode_inquery_handler(void){

    if(packet_payload_length != 1){
        return;
    }
    
    uint8_t subcode = packet_buffer[4];
    switch (subcode)
    {
        case INQUERY_SUBCODE_VERSION:
            bl_response(PACKET_OPCODE_INQUERY, PACKET_ERRCODE_OK, (uint8_t *)BL_VERSION, strlen(BL_VERSION));
            break;
        case INQUERY_SUBCODE_MTU:
				{
            uint8_t bmtu[2] = {PAYLOAD_SIZE_MAX & 0xFF, (PAYLOAD_SIZE_MAX >> 8) & 0xFF};//小端发送大小
            bl_response(PACKET_OPCODE_INQUERY, PACKET_ERRCODE_OK, (uint8_t *)&bmtu, sizeof(bmtu));
            break;
				}
        default:
            break;
    }
    

}


static void bl_opcode_erase_handler(void)
{
    log_i("erase handler");

    if (packet_payload_length != 8)
    {
        log_e("erase packet length error: %d", packet_payload_length);
        bl_response(PACKET_OPCODE_ERASE, PACKET_ERRCODE_PARAM, NULL, 0);
        return;
    }
    //上位机小端发送地址和size，因为低位在前，所以直接接收就行
    uint32_t address = (packet_buffer[7] << 24) | (packet_buffer[6] << 16) | (packet_buffer[5] << 8) | packet_buffer[4];
    uint32_t size = (packet_buffer[11] << 24) | (packet_buffer[10] << 16) | (packet_buffer[9] << 8) | packet_buffer[8];

    if (address < STM32_FLASH_BASE || address + size > STM32_FLASH_BASE + STM32_FLASH_SIZE)
    {
        log_w("erase address=0x%08X, size=%u out of range", address, size);
        bl_response(PACKET_OPCODE_ERASE, PACKET_ERRCODE_PARAM, NULL, 0);
        return;
    }

    if (address >= BL_ADDRESS && address < BL_ADDRESS + BL_SIZE)
    {
        log_w("address 0x%08X is protected", address);
        bl_response(PACKET_OPCODE_ERASE, PACKET_ERRCODE_PARAM, NULL, 0);
        return;
    }

    log_d("erase address=0x%08X, size=%u", address, size);

    stm32_flash_unlock();
    stm32_flash_erase(address, size);
    stm32_flash_lock();

    bl_response(PACKET_OPCODE_ERASE, PACKET_ERRCODE_OK, NULL, 0);
}

static void bl_opcode_program_handler(void)
{
    log_i("program handler");

    if (packet_payload_length <= 8)
    {
        log_e("program packet length error: %d", packet_payload_length);
        bl_response(PACKET_OPCODE_PROGRAM, PACKET_ERRCODE_PARAM, NULL, 0);
        return;
    }

    uint32_t address = (packet_buffer[7] << 24) | (packet_buffer[6] << 16) | (packet_buffer[5] << 8) | packet_buffer[4];
    uint32_t size = (packet_buffer[11] << 24) | (packet_buffer[10] << 16) | (packet_buffer[9] << 8) | packet_buffer[8];
    uint8_t *data = &packet_buffer[12];

    if (address < STM32_FLASH_BASE || address + size > STM32_FLASH_BASE + STM32_FLASH_SIZE)
    {
        log_w("program address=0x%08X, size=%u out of range", address, size);
        bl_response(PACKET_OPCODE_PROGRAM, PACKET_ERRCODE_PARAM, NULL, 0);
        return;
    }

    if (address >= BL_ADDRESS && address < BL_ADDRESS + BL_SIZE)
    {
        log_w("address 0x%08X is protected", address);
        bl_response(PACKET_OPCODE_PROGRAM, PACKET_ERRCODE_PARAM, NULL, 0);
        return;
    }

    if (size != packet_payload_length - 8)
    {
        log_w("program size %u does not match payload length %u", size, packet_payload_length - 8);
        bl_response(PACKET_OPCODE_PROGRAM, PACKET_ERRCODE_PARAM, NULL, 0);
        return;
    }

    log_d("program address=0x%08X, size=%u", address, size);

    stm32_flash_unlock();
    stm32_flash_program(address, data, size);
    stm32_flash_lock();

    bl_response(PACKET_OPCODE_PROGRAM, PACKET_ERRCODE_OK, NULL, 0);
}

static void bl_opcode_verify_handler(void)
{
    log_i("verify handler");

    if (packet_payload_length != 12)
    {
        log_e("verify packet length error: %d", packet_payload_length);
        bl_response(PACKET_OPCODE_VERIFY, PACKET_ERRCODE_PARAM, NULL, 0);
        return;
    }

    uint32_t address = (packet_buffer[7] << 24) | (packet_buffer[6] << 16) | (packet_buffer[5] << 8) | packet_buffer[4];
    uint32_t size = (packet_buffer[11] << 24) | (packet_buffer[10] << 16) | (packet_buffer[9] << 8) | packet_buffer[8];
    uint32_t crc = (packet_buffer[15] << 24) | (packet_buffer[14] << 16) | (packet_buffer[13] << 8) | packet_buffer[12];

    if (address < STM32_FLASH_BASE || address + size > STM32_FLASH_BASE + STM32_FLASH_SIZE)
    {
        log_w("verify address=0x%08X, size=%u out of range", address, size);
        bl_response(PACKET_OPCODE_VERIFY, PACKET_ERRCODE_PARAM, NULL, 0);
        return;
    }

    log_d("verify address=0x%08X, size=%u, crc=0x%08X", address, size, crc);

    uint32_t ccrc = crc32((uint8_t *)address, size);

    if (ccrc != crc)
    {
        log_e("verify failed: expected 0x%08X, got 0x%08X", crc, ccrc);
        bl_response(PACKET_OPCODE_VERIFY, PACKET_ERRCODE_VERIFY, NULL, 0);
        return;
    }

    bl_response(PACKET_OPCODE_VERIFY, PACKET_ERRCODE_OK, NULL, 0);
}

static void bl_opcode_reset_handler(void)
{
    log_i("reset handler");
    bl_response(PACKET_OPCODE_RESET, PACKET_ERRCODE_OK, NULL, 0);
    log_i("system resetting...");
    tim_delay_ms(2);

    NVIC_SystemReset();
}


static void bl_opcode_boot_handler(void)
{
    log_i("boot handler");
    bl_response(PACKET_OPCODE_BOOT, PACKET_ERRCODE_OK, NULL, 0);
    log_i("booting application...");

    boot_application();
}


//接收完数据包后，单片机要支持响应 回去数据
static void bl_packet_handler(void)
{
    switch (packet_opcode)
    {
        case PACKET_OPCODE_INQUERY:
            bl_opcode_inquery_handler();
            break;
        case PACKET_OPCODE_ERASE:
            bl_opcode_erase_handler();
            break;
        case PACKET_OPCODE_PROGRAM:
            bl_opcode_program_handler();
            break;
        case PACKET_OPCODE_VERIFY:
            bl_opcode_verify_handler();
            break;
        case PACKET_OPCODE_RESET:
            bl_opcode_reset_handler();
            break;
        case PACKET_OPCODE_BOOT:
            bl_opcode_boot_handler();
            break;
        default:
            // 未知指令
            log_e("Unknown command: %02X", packet_opcode);
            break;
    }
}



/**
 * 经典的串口包
*/
static bool bl_byte_handler(uint8_t data)
{
    bool full_packet = false;//加一个标志用于判断是否串口包完全发送

    //超时机制
    // 处理字节数据超时接收
    static uint64_t last_byte_ms;
    uint64_t now_ms = tim_get_ms();
    if (now_ms - last_byte_ms > RX_TIMEOUT_MS)
    {
        if (packet_state != PACKET_STATE_HEADER)
            log_w("last packet rx timeout");
        packet_index = 0;
        packet_state = PACKET_STATE_HEADER;
    }
    last_byte_ms = now_ms;


    // 字节接收状态机处理
    log_v("recv: %02X", data);
    packet_buffer[packet_index++] = data;
    switch (packet_state)
    {
        case PACKET_STATE_HEADER:
            if (packet_buffer[0] == 0xAA)
            {
                log_d("header ok");
                packet_state = PACKET_STATE_OPCODE;
            }
            else
            {
                packet_index = 0;
                packet_state = PACKET_STATE_HEADER;
            }
            break;
        case PACKET_STATE_OPCODE:
            if (packet_buffer[1] == PACKET_OPCODE_INQUERY ||
                packet_buffer[1] == PACKET_OPCODE_ERASE ||
                packet_buffer[1] == PACKET_OPCODE_PROGRAM ||
                packet_buffer[1] == PACKET_OPCODE_VERIFY ||
                packet_buffer[1] == PACKET_OPCODE_RESET ||
                packet_buffer[1] == PACKET_OPCODE_BOOT)
            {
                log_d("opcode ok: %02X", packet_buffer[1]);
                packet_opcode = (packet_opcode_t)packet_buffer[1];
                packet_state = PACKET_STATE_LENGTH;
            }
            else
            {
                packet_index = 0;
                packet_state = PACKET_STATE_HEADER;
            }
            break;
        case PACKET_STATE_LENGTH:
            if (packet_index == 4)//注意前面的两个状态+当前的lenth状态 达到4字节
            {
                uint16_t payload_length = (packet_buffer[3] << 8) | packet_buffer[2];//接收到的两字节高低位转换，存放到一个2字节的数组里面
                if (payload_length <= PACKET_SIZE_MAX )
                {
                    log_d("length ok: %u", payload_length);
                    packet_payload_length = payload_length;
                    
                    if(packet_payload_length > 0){
                        packet_state = PACKET_STATE_PAYLOAD;
                    }else{
                        packet_state = PACKET_STATE_CRC16;
                    }
                    
                }
                else
                {
                    packet_index = 0;
                    packet_state = PACKET_STATE_HEADER;
                }
            }
            break;
        case PACKET_STATE_PAYLOAD:
            if (packet_index == 4 + packet_payload_length)
            {

                log_d("payload receive ok");
                packet_state = PACKET_STATE_CRC16;
                
            }
            break;
        case PACKET_STATE_CRC16:  //加上crc的校验判断
            if (packet_index == 4 + packet_payload_length + 2)
            {
                 uint16_t crc = (packet_buffer[4 + packet_payload_length + 1] << 8) |
                                packet_buffer[4 + packet_payload_length];
                uint16_t ccrc = crc16(packet_buffer, 4 + packet_payload_length);
                if (crc == ccrc)
                {
                    full_packet = true;
                    log_d("crc16 ok: %04X", crc);
                    log_d("packet received: opcode=%02X, length=%u", packet_opcode, packet_payload_length);
                    if(LOG_LVL >= ELOG_LVL_VERBOSE){
                        elog_hexdump("payload",10,packet_buffer,4 + packet_payload_length);
                    }

                }
                else
                {
                    log_e("crc16 error: expected %04X, got %04X", crc, ccrc);
                }

                packet_index = 0;
                packet_state = PACKET_STATE_HEADER;
            }
            break;
        default:
            break;
    }
    return full_packet;
}

static void bl_usart_rx_handler(const uint8_t *data, uint32_t length)
{
    rb_puts(rxrb, data, length);
}

//判断按键是否按下超过3s
static bool key_trap_check(void)
{
    for (uint32_t t = 0; t < BOOT_DELAY; t+=10)
    {
        tim_delay_ms(10);
        if (!key_read(key1)) //按下是true  取反是没按下，所以这里直接认为没按下就行
            return false;
    }
    log_w("key pressed, trap into boot");
    return true;
}

//判断是否松开了
static void wait_key_release(void)
{
    while (key_read(key1))
        tim_delay_ms(10);
}


//boot中途判断key是否按下
static bool key_press_check(void)
{
    if (!key_read(key1))
        return false;

    tim_delay_ms(10);
    if (!key_read(key1))
        return false;

    return true;
}

bool magic_header_trap_boot(void)
{
    if (!magic_header_validate())
    {
        log_w("magic header invalid, trap into boot");
        return true;
    }

    if (!application_validate())
    {
        log_e("application validate failed, trap into boot");
        return true;
    }

    return false;
}

//这个主要是方便在reset后的3s内实现串口升级的功能，即不管按键按没按
//复位后的3s都会死等待，如果3s内ringbuffer有数据就直接进入boot，方便后续调试
bool rx_trap_boot(void)
{
    for (uint32_t i = 0; i < 3000; i += 10)
    {
        tim_delay_ms(10);
        if (!rb_empty(rxrb))
        {
            log_w("data received, trap into boot");
            return true;
        }
    }

    return false;
}


//bootloader的main程序
int bootloader_main(void)
{
		
    //bootloader模式
    rxrb = rb_new(rb_buffer,RX_BUFFER_SIZE);
    log_i("Bootloader started.");


    //注册回调实现串口的rx触发一次RXNE的中断就放这个数据到ringbuffer的缓存里面
    bl_usart_received_register(bl_usart_rx_handler);


    //增加按键boot模式
    key_init(key1);

    bool trapboot = key_trap_check();
		
	
    if (!trapboot)
        trapboot = magic_header_trap_boot();

#ifdef third_wait_boot
    if (!trapboot)//方便抢占升级
        trapboot = rx_trap_boot();
#endif

    if (!trapboot)
    {
        boot_application();
    }

    led_init(led1);
    led_on(led1);
    wait_key_release();

    while(1){

        if (key_press_check())//boot模式判断中途是否key按下
        {
            log_i("key pressed, rebooting...");
            tim_delay_ms(2);
            NVIC_SystemReset();
        }

        if(!rb_empty(rxrb)){
            
            uint8_t byte;
            rb_get(rxrb, &byte);
            if (bl_byte_handler(byte))
            {
                bl_packet_handler();
            }

        }

    }

    
}
