#include <Arduino.h>
#include "udc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "esp_task_wdt.h"
// #include "soc/rtc_wdt.h"    // 设置看门狗应用

void udc_uart_rev( void * param );
void udc_function( void * param );

udc_pack_t udc_pack;
static uint8_t udc_pack_receive_buffer[300];
static uint8_t udc_pack_send_buffer[300];

static udc_event_dsc_t rev_finsh_cb;
static udc_event_dsc_t rev_finsh_cb2;

static int send_bytes_func(const struct _udc_pack_t *pack, const uint8_t *buf, uint16_t len)
{
    Serial.write(buf, len);
    return len;
}
// 接收完成回调
void event_cb(udc_event_t * e)
{
    static uint8_t data_buf[50] = {0};
    udc_pack_t * pack = udc_event_get_target(e);
    udc_obj_t obj;

    UDC_PACK_OBJ_FOREACH(0, pack, &obj, // 接收完成，对数据obj迭代
        Serial.printf("obj id:%d, obj len:%d\n", obj.id, obj.size);
    );
    Serial.printf("udc pack task\n");
    UDC_PACK_RECEIVE_WAIT(pack, data_buf, 50, 2000, // 组测等待接收
        udc_obj_t obj;
        UDC_PACK_OBJ_FOREACH(0, pack, &obj,
            Serial.printf(">>> obj id:%d, obj len:%d\n", obj.id, obj.size);
        );
        UDC_PACK_RECEIVE_WAIT(pack, data_buf, 50, 2000, 
            udc_obj_t obj;
            UDC_PACK_OBJ_FOREACH(0, pack, &obj,
                Serial.printf("<<< obj id:%d, obj len:%d\n", obj.id, obj.size);
            );
        );
    );
}

void event_cb2(udc_event_t * e)
{
    udc_pack_t * pack = udc_event_get_target(e);
    udc_obj_t obj;

    UDC_PACK_OBJ_FOREACH(0, pack, &obj,
        Serial.printf("2222>> obj id:%d, obj len:%d\n", obj.id, obj.size);
    );
}

void loop(){
    vTaskDelay(500 / portTICK_PERIOD_MS);
}
void setup()
{
    Serial.begin(115200);

    udc_pack_init_t udc_pack_init_temp = {
        .pack = &udc_pack,
        .receive_buffer = udc_pack_receive_buffer,
        .transmit_buffer = udc_pack_send_buffer,
        .receive_buffer_size = sizeof(udc_pack_receive_buffer),
        .transmit_buffer_size = sizeof(udc_pack_send_buffer),
        .is_transmit_buffer_dynamic = 0,
        .is_receive_buffer_dynamic = 0,
    };
    udc_pack_init(&udc_pack_init_temp);
    udc_pack_set_send_bytes_func(&udc_pack, send_bytes_func);
    udc_pack_add_event_cb_static(&udc_pack, &rev_finsh_cb, event_cb, UDC_EVENT_PACK_RECEIVE_FINSHED, NULL);
    udc_pack_add_event_cb_static(&udc_pack, &rev_finsh_cb2, event_cb2, UDC_EVENT_PACK_RECEIVE_FINSHED, NULL);

    uint8_t temp_data[10] = {99};
    udc_pack_push_sigal(&udc_pack, 1, 1, temp_data);

    vTaskDelay(1000);
    xTaskCreate(udc_uart_rev, "udc_rev", 2048, NULL, 5, NULL);
    xTaskCreate(udc_function, "udc", 2048, NULL, 4, NULL);
}

void udc_uart_rev( void * param )
{
    static uint8_t rev_buf[30];
    while(1)
    {
        if(Serial.available())
        {
            uint16_t len =  Serial.readBytes(rev_buf, sizeof(rev_buf));
            udc_pack_receive_data(&udc_pack, rev_buf, len);
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
