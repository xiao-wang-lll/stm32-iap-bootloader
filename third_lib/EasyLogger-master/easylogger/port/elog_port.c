#include <elog.h>
#include "tim_delay.h"
#include <stdio.h>
#include "console.h"
#include "bl_usart.h"




ElogErrCode elog_port_init(void) {
    
        return ELOG_NO_ERR;

}


void elog_port_deinit(void) {

   
}


void elog_port_output(const char *log, size_t size) {
    
    //console_write(log, size);
    bl_usart_write((uint8_t *)log,size);
}


void elog_port_output_lock(void) {
    
    
}
    


void elog_port_output_unlock(void) {
    
    
    
}


const char *elog_port_get_time(void) {
    
    
    uint64_t total_seconds = tim_get_ms();
    static char time_str[16] = {0};
    uint32_t ms = tim_get_ms() % (3600 * 1000); //限制时间为1h，即ms数抛弃整数个小时后剩余的ms时间
    uint32_t fmt_mm = ms / (60 * 1000);
    uint32_t fmt_ss = (ms % (60 * 1000)) / 1000;
    uint32_t fmt_ms = ms % 1000;
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%03d", fmt_mm, fmt_ss, fmt_ms);
    return time_str;
    
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    
    return "";
    
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    
   
    return "";
}


