#include <stdio.h>
#include "udc.h"

udc_pack_t pack;
uint8_t pack_send_buf[100];
uint8_t pack_recv_buf[100];

udc_pack_init_t pack_init = {
    .pack = &pack,
    .header = 
    {
        .header = "\x00\x01\x02",
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

int main()
{
    udc_pack_init(&pack_init);
    // udc_pack_set_buffer_static(&pack, 0, pack_recv_buf, 100);
    udc_pack_set_buffer_static(&pack, 1, pack_send_buf, 100);
    udc_pack_set_send_bytes_func(&pack, send_bytes_func);
    udc_pack_append_data(&pack, 0, 5, "hello");

    udc_pack_task();
    return 0;
}
