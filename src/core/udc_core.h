#ifndef __UDC_CORE_H__
#define __UDC_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../udc_conf.h"
#include "stdbool.h"
#include "udc_event.h"

struct _udc_pack_t;

// return lenth of data
typedef int (*udc_send_bytes_func_t)(uint8_t *buf, uint16_t len);

#define UDC_PACK_OBJ_FOREACH(revive_or_transmit, pack,  obj, active)         \
    udc_pack_get_first_obj(pack, revive_or_transmit, obj);                   \
    do                                                                       \
    {                                                                        \
        active                                                               \
    } while (0 == udc_pack_get_next_obj(pack, revive_or_transmit, obj, obj)) \
    

typedef struct _udc_obj_t
{
    uint8_t  *data;
    uint16_t  size;  // data len
    uint8_t   id;        // data id
    uint8_t   pack_id;   // pack_id=id+120 when size>255, otherwise pack_id=id
} udc_obj_t;


typedef struct _udc_receive_t
{
    uint8_t *target_buf;
    uint8_t buffer[UDC_MEM_SIZE];
    uint16_t padding_size;
    uint16_t buffer_size;
    uint8_t target_buf_is_dynamic : 1; //  Whether the buffer is dynamically applied
    uint8_t receive_finished : 1; //  receive finished
    uint32_t recevice_last_tick;
} udc_receive_t;


/*For future DMA adaptation, all data for a package is stored in a buffer*/
typedef struct _udc_transmit_t
{
    uint8_t *target_buf;
    uint8_t buffer[UDC_MEM_SIZE];
    uint16_t padding_size;
    uint16_t buffer_size;
    uint8_t target_buf_is_dynamic : 1; //  Whether the buffer is dynamically applied
} udc_transmit_t;


typedef struct 
{
    udc_event_dsc_t * event_dsc;

    uint8_t event_dsc_cnt;
} _udc_obj_spec_attr_t;



typedef struct _udc_pack_t udc_pack_t;
struct _udc_pack_t
{
    udc_pack_t *next;
    int  (*send_bytes)(uint8_t *buf, uint16_t len);
    _udc_obj_spec_attr_t spec_attr;
    udc_receive_t receive;
    udc_transmit_t transmit;
};

void udc_pack_init(udc_pack_t *pack);
void udc_pack_receive_data(udc_pack_t *pack, const uint8_t *buf, uint16_t len);
void udc_pack_task(void);
// a simple wait, not verify check ...
int  udc_pack_receive_wait(udc_pack_t *pack, uint8_t is_clean_finsh, uint32_t timeout);

void udc_pack_set_send_bytes_func(udc_pack_t *pack, udc_send_bytes_func_t send_bytes);
int  udc_pack_set_buffer_size(udc_pack_t *pack, uint8_t receive_or_transmit, uint16_t buffer_size);
void udc_pack_recover_buffer(udc_pack_t *pack, uint8_t receive_or_transmit);


int  udc_pack_append_data(udc_pack_t *pack, uint8_t id, uint16_t size, const void *data);
int  udc_pack_push(udc_pack_t *pack);
// The cache comes from the stack and does not trigger 'UDC_EVENT_PACK_TRANSMIT_FINSHED' events
int  udc_pack_push_sigal(udc_pack_t *pack,  uint8_t id, uint16_t size, const void *data);

uint16_t     udc_pack_get_padding_size(const udc_pack_t *pack, uint8_t revive_or_transmit);
udc_pack_t * udc_pack_get_header(void);
udc_pack_t * udc_pack_get_next(udc_pack_t * pack);


bool udc_pack_obj_is_end(const udc_pack_t *pack, uint8_t revive_or_transmit, udc_obj_t *obj);
int  udc_pack_get_first_obj(udc_pack_t *pack, uint8_t revive_or_transmit, udc_obj_t *obj);
int  udc_pack_get_next_obj(udc_pack_t *pack, uint8_t revive_or_transmit, udc_obj_t *base_obj, udc_obj_t *next_obj);
int  udc_pack_get_obj(udc_pack_t *pack, uint8_t revive_or_transmit, uint8_t id, udc_obj_t *obj);


#ifdef __cplusplus
}
#endif

#endif // __UDCHECK_H__
