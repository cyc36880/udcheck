#include <Arduino.h>
#include "udc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp_task_wdt.h"

udc_pack_t udc_pack;
udc_pack_t udc_pack2;
uint8_t udc_send_buf[100];
uint8_t udc_recv_buf[100];

uint8_t udc_send_buf2[100];
uint8_t udc_recv_buf2[100];

void udc_uart_rev( void * param );
void udc_function( void * param );

static int send_bytes_func(const struct _udc_pack_t *pack, const uint8_t *buf, uint16_t len)
{
    Serial.write(buf, len);
    return len;
}

void udc_event_cb(udc_event_t * e)
{
    static uint8_t data_buf[50] = {0};
    udc_pack_t * pack  = udc_event_get_target(e);
    udc_event_code_t code = udc_event_get_code(e);
    udc_obj_t obj;
    Serial.printf("code: %d, pack: %p\n", code, pack);
    
    UDC_PACK_OBJ_FOREACH(UDC_PACK_RECEIVE, pack, &obj,  // 接收完成，对数据obj迭代
        Serial.printf("size: %d, id: %d \n", obj.size, obj.id);
    );
    Serial.printf("udc pack task\n");
    UDC_PACK_RECEIVE_WAIT_0(pack, data_buf, 50, 2000, // 阻塞等待接收
        udc_obj_t obj;
        UDC_PACK_OBJ_FOREACH(UDC_PACK_RECEIVE, pack, &obj,
            Serial.printf(">>>1 obj id:%d, obj len:%d\n", obj.id, obj.size);
        );
        UDC_PACK_RECEIVE_WAIT_0(pack, data_buf, 50, 2000,  // 阻塞等待接收
            udc_obj_t obj;
            UDC_PACK_OBJ_FOREACH(UDC_PACK_RECEIVE, pack, &obj,
                Serial.printf("<<<2 obj id:%d, obj len:%d\n", obj.id, obj.size);
            );
        );
    );
    Serial.printf("udc pack task exit\n");
}
void udc_event_cb2(udc_event_t * e)
{
    udc_pack_t * pack  = udc_event_get_target(e);
    udc_event_code_t code = udc_event_get_code(e);
    udc_obj_t obj;
    
    UDC_PACK_OBJ_FOREACH(UDC_PACK_RECEIVE, pack, &obj, 
        Serial.printf("size: %d, id: %d \n", obj.size, obj.id);
    );
}

void setup()
{
    Serial.begin(115200);
    udc_pack_init_t init = {
        .pack = &udc_pack,
        .header = {
            .header = "\x01\x02\x03\x04",
            .header_len = 4,
        }
    };
    udc_pack_init_t init2 = {
        .pack = &udc_pack2,
        .header = {
            .header = "\x01\x02\x03\x05",
            .header_len = 4,
        }
    };
    udc_pack_init(&init);
    udc_pack_init(&init2);

    udc_pack_set_send_bytes_func(&udc_pack, send_bytes_func);
    udc_pack_set_buffer_static(&udc_pack, UDC_PACK_RECEIVE, udc_send_buf, sizeof(udc_send_buf));
    udc_pack_set_buffer_static(&udc_pack, UDC_PACK_TRANSMIT, udc_recv_buf, sizeof(udc_recv_buf));
    udc_pack_add_event_cb(&udc_pack, udc_event_cb, UDC_EVENT_PACK_RECEIVE_FINSHED, NULL);

    udc_pack_set_send_bytes_func(&udc_pack2, send_bytes_func);
    udc_pack_set_buffer_static(&udc_pack2, UDC_PACK_RECEIVE, udc_send_buf2, sizeof(udc_send_buf));
    udc_pack_set_buffer_static(&udc_pack2, UDC_PACK_TRANSMIT, udc_recv_buf2, sizeof(udc_recv_buf));
    udc_pack_add_event_cb(&udc_pack2, udc_event_cb2, UDC_EVENT_PACK_RECEIVE_FINSHED, NULL);

    udc_pack_append_data(&udc_pack, 0, 5, "hello");
    udc_pack_append_data(&udc_pack, 1, 5, "hello");
    delay(50);
    udc_pack_append_data(&udc_pack2, 0, 5, "hello");
    delay(50);

    xTaskCreate(udc_uart_rev, "udc_rev", 2048, NULL, 5, NULL);
    xTaskCreate(udc_function, "udc", 2048, NULL, 4, NULL);
}

void loop()
{}

void udc_uart_rev( void * param )
{
    static uint8_t rev_buf[30];
    while(1)
    {
        if(Serial.available())
        {
            uint16_t len =  Serial.readBytes(rev_buf, sizeof(rev_buf));
            udc_pack_receive_data(&udc_pack, rev_buf, len);
            udc_pack_receive_data(&udc_pack2, rev_buf, len);
        }
        esp_task_wdt_reset();
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void udc_function( void * param )
{
    while (1)
    {
        udc_pack_task();
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}