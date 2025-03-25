#include <stdio.h>

#include "udc.h"

static udc_event_dsc_t event_dsc_0;
static udc_pack_t udc_pack;

int udc_send_bytes(uint8_t *buf, uint16_t len)
{
    for (uint16_t i=0; i<len; i++)
    {
        printf("%#X ", buf[i]);
    }
    return 0;
}

static const uint8_t test_buf[] = {
    0XFA, 0XD, 0, 0X2, 0X1, 0X33, 0X34, 0X1, 0X1, 0X31, 0X32, 0X3, 0X1, 0X33, 0X34, 0X41
};



void event_cb(udc_event_t * e)
{
    udc_pack_t * pack = udc_event_get_target(e);
    udc_obj_t obj;

    udc_pack_get_first_obj(pack, 0, &obj);
    for ( ; ; )
    {
        printf("id %d, type %d\n", obj.id, obj.data_type);
        if (0 != udc_pack_get_next_obj(pack, 0,  &obj, &obj)) break;
    }

    printf("event_cb: %d\n", udc_event_get_code(e));
}


int main()
{
    udc_pack_init(&udc_pack);
    udc_set_send_bytes_func(&udc_pack, udc_send_bytes);
    udc_pack_add_event_cb_static(&event_dsc_0, &udc_pack, event_cb, UDC_EVENT_PACK_RECEIVE_FINSHED, NULL);

    udc_append_data(&udc_pack, 2, DATTYPE_2byte, "34");
    udc_append_data(&udc_pack, 1, DATTYPE_2byte, "12");
    udc_append_data(&udc_pack, 3, DATTYPE_2byte, "34");
    udc_append_data(&udc_pack, 3, DATTYPE_2byte, "99");
    udc_pack_push(&udc_pack);

    udc_pack_receive_data(&udc_pack, test_buf, sizeof(test_buf));
    udc_pack_task();

    return 0;
}


