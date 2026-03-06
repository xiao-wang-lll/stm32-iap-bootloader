#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "ringbuffer.h"

//ringfbuffer
struct ringbuffer
{
	uint16_t size;
	uint16_t head;
	uint16_t tail;

	//使用柔性数组
	uint8_t buffer[];

};


rb_t rb_new(uint8_t *buffer, uint32_t length)
{
    if (length < sizeof(struct ringbuffer) + 1)
        return NULL;

    rb_t rb = (rb_t)buffer;
    rb->head = 0;
    rb->tail = 0;
    rb->size = length - sizeof(struct ringbuffer);//减去当前的head和tail和size所占据的内存剩下的就是buffer可以使用的字节长度

    return rb;
}

//判空和判满
//将原本的
static inline uint16_t next_head(rb_t rb)
{
    return rb->head + 1 < rb->size ? rb->head + 1 : 0;
}

static inline uint16_t next_tail(rb_t rb)
{
    return rb->tail + 1 < rb->size ? rb->tail + 1 : 0;
}

bool rb_empty(rb_t rb)
{
    return rb->head == rb->tail;
}

bool rb_full(rb_t rb)
{
    return next_head(rb) == rb->tail;
}

//放入数据 和 取数据---单字节

bool rb_put(rb_t rb, uint8_t data)
{
    if (rb_full(rb))
        return false;

    rb->buffer[rb->head] = data;
    rb->head = next_head(rb);

    return true;
}

bool rb_get(rb_t rb, uint8_t *data)
{
    if (rb_empty(rb))
        return false;

    *data = rb->buffer[rb->tail];
    rb->tail = next_tail(rb);

    return true;
}


//放入数据 和 取数据---连续的多个字节
bool rb_puts(rb_t rb, const uint8_t *data, uint32_t length)
{
    while (length--)
    {
        if (!rb_put(rb, *data++))
            return false;
    }
    return true;
}

uint32_t rb_gets(rb_t rb, uint8_t *data, uint32_t length)
{
    uint32_t count = 0;
    while (length--)
    {
        if (!rb_get(rb, data++))
            break;
        count++;
    }
    return count;
}

