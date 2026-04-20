#include <stdio.h>
#include "udc.h"

udc_pack_group_t pack_group;

udc_pack_t pack;
uint8_t pack_send_buf[100];
uint8_t pack_recv_buf[100];

udc_pack_t pack1;
uint8_t pack_send_buf1[100];
uint8_t pack_recv_buf1[100];

udc_pack_init_t pack_init = {
    .pack_group = &pack_group,
    .pack = &pack,
    .header = 
    {
        .header = "\x00\x01\x02",
        .header_len = 3,
    },
};

udc_pack_init_t pack_init1 = {
    .pack_group = &pack_group,
    .pack = &pack1,
    .header = 
    {
        .header = "\x00\x01\x03",
        .header_len = 3,
    },
};


int send_bytes_func(const struct _udc_pack_t *pack, const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        printf("0x%02x, ", buf[i]);
    }
    return len;
}

void event_callback(udc_event_t * e)
{
    udc_event_code_t code = udc_event_get_code(e);
    void * user_data = udc_event_get_user_data(e);
    printf("\nevent_callback: user_data->(%d), code->(%d)\n", (uintptr_t)user_data, code);
}

int main()
{
    /**
     * 初始化
     */
    udc_pack_init(&pack_init);
    udc_pack_set_buffer_static(&pack, UDC_PACK_RECEIVE, pack_recv_buf, 100);
    udc_pack_set_buffer_static(&pack, UDC_PACK_TRANSMIT, pack_send_buf, 100);

    udc_pack_init(&pack_init1);
    udc_pack_set_buffer_static(&pack1, UDC_PACK_RECEIVE, pack_recv_buf1, 100);
    udc_pack_set_buffer_static(&pack1, UDC_PACK_TRANSMIT, pack_send_buf1, 100);

    udc_pack_set_send_bytes_func(&pack, send_bytes_func);
    udc_pack_set_send_bytes_func(&pack1, send_bytes_func);

    /**
     * 添加事件 (需要使能 alloc 功能)
     */
    udc_pack_add_event_cb(&pack, event_callback, UDC_EVENT_ALL, (void*)1);
    udc_pack_add_event_cb(&pack1, event_callback, UDC_EVENT_ALL, (void*)2);


    /**
     * 向pack中添加数据
     */
    udc_pack_append_data(&pack, 0, 5, "hello");
    udc_pack_append_data(&pack1, 0, 2, "12");

    uint8_t receive_data[] = {0x00, 0x01, 0x03, 0x00, 0x05, 0x00, 0x02, 0x31, 0x32, 0x6e};
    udc_pack_receive_data(&pack_group, receive_data, sizeof(receive_data));



    udc_pack_task(&pack_group);
    return 0;
}
