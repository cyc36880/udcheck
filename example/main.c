#include "stdio.h"
#include "udc.h"

static uint8_t udc_rev_buf[300];
static uint8_t udc_transmit_buf[300];
udc_pack_t udc_pack;

/**
 * 这里向终端中发送数据，以便观察
 * 实际应该将原始数据（16进制）发送出去
 */
int send_bytes_func(const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        printf("0x%02x, ", buf[i]);
    }
    return len;
}

const uint8_t test_data[300] = {0};

const uint8_t rev_buf[] = {0xfa, 0x00, 0x04, 0x0a, 0x01, 0x00, 0x09};

udc_event_dsc_t event1;

void event_cb(udc_event_t * e)
{
    udc_pack_t * p = udc_event_get_target(e);
    udc_obj_t obj;
    UDC_PACK_OBJ_FOREACH(0, p, &obj,
        printf("\nobj id: %d, len: %d\n", obj.id, obj.size);
    );
}



int main()
{
    udc_pack_init_t udc_pack_init_temp = {
        .pack = &udc_pack,
        .transmit_buffer = udc_transmit_buf,
        .transmit_buffer_size = sizeof(udc_transmit_buf),
        .receive_buffer = udc_rev_buf,
        .receive_buffer_size = sizeof(udc_rev_buf),
        .is_receive_buffer_dynamic = 0,
        .is_transmit_buffer_dynamic = 0
    };
    udc_pack_init(&udc_pack_init_temp);
    udc_pack_set_send_bytes_func(&udc_pack, send_bytes_func);
    udc_pack_add_event_cb_static(&udc_pack, &event1, event_cb, UDC_EVENT_PACK_RECEIVE_FINSHED, NULL);

    udc_pack_append_data(&udc_pack, 10, 1, test_data);
    udc_pack_push(&udc_pack);


    udc_pack_receive_data(&udc_pack, rev_buf, sizeof(rev_buf));
    udc_pack_task();

    uint8_t rev_buf[30];
    UDC_PACK_RECEIVE_WAIT( (&udc_pack), rev_buf, 30, 1000, 
        printf("123");
    );

    return 0;
}
