#include <stdio.h>

#include "udc.h"

static udc_event_dsc_t event_dsc_0;
static udc_pack_t udc_pack;

/**
 * 这里向终端中发送数据，以便观察
 * 实际应该将原始数据（16进制）发送出去
 */
int udc_send_bytes(uint8_t *buf, uint16_t len)
{
    for (uint16_t i=0; i<len; i++)
    {
        printf("%#X ", buf[i]);
    }
    return len;
}

static const uint8_t test_buf[] = {
    0XFA, 0XD, 0, 0X2, 0X1, 0X33, 0X34, 0X1, 0X1, 0X31, 0X32, 0X3, 0X1, 0X33, 0X34, 0X41
};

// 事件回调
void event_cb(udc_event_t * e)
{
    udc_pack_t * pack = udc_event_get_target(e);
    udc_obj_t obj;

    UDC_PACK_OBJ_FOREACH(0, pack, &obj,
        printf("id %d, size %d\n", obj.id, obj.size);
        printf("obj data: ");
        for (uint16_t i=0; i<obj.size; i++)
        {
            printf("%#X ", obj.data[i]);
        }
        printf("\n----------\n");
    );

    printf("event_cb: %d\n", udc_event_get_code(e));
}


int main()
{
    udc_pack_init(&udc_pack);
    udc_pack_set_send_bytes_func(&udc_pack, udc_send_bytes);
    udc_pack_add_event_cb_static(&event_dsc_0, &udc_pack, event_cb, UDC_EVENT_PACK_RECEIVE_FINSHED, NULL);

    udc_pack_append_data(&udc_pack, 2, 2, "34");
    udc_pack_append_data(&udc_pack, 1, 2, "12");
    udc_pack_append_data(&udc_pack, 3, 2, "34");
    udc_pack_append_data(&udc_pack, 3, 2, "99");
    udc_pack_push(&udc_pack);

    udc_pack_receive_data(&udc_pack, test_buf, sizeof(test_buf));
    udc_pack_task();

    return 0;
}


