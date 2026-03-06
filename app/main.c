#include "elog.h"
#include "tim_delay.h"
#include "bl_usart.h"

#define APP_BASE_ADDRESS    0x08010000

extern int bootloader_main(void);
extern void board_lowlevel_init(void);
int main(void)
{
    board_lowlevel_init();
		bl_usart_init();
    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME | ELOG_FMT_T_INFO);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_start();

    //console_init();因为我就一个串口 所以不用这个，用上位机传指令的串口3
		tim_delay_init();
    bootloader_main();



    return 0;
}
