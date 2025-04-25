#ifndef __UDC_CORE_H__
#define __UDC_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../udc_conf.h"
#include "stdbool.h"
#include "udc_event.h"

struct _udc_pack_t;

typedef int (*udc_send_bytes_func_t)(uint8_t *buf, uint16_t len);

typedef enum
{
	DATTYPE_1byte=0,
	DATTYPE_2byte,
	DATTYPE_4byte,
	DATTYPE_8byte,
	DATTYPE_16byte,
	DATTYPE_32byte,
} UDC_DATA_TYPE;


typedef struct _udc_obj_t
{
    uint8_t  *data;
    uint16_t  data_type; // data type
    uint8_t   id;        // data id
} udc_obj_t;


typedef struct _udc_receive_t
{
    uint8_t *target_buf;
    uint8_t buffer[UDC_MEM_SIZE];
    UDC_DATA_TYPE_LIST_TYPE padding_size;
    UDC_DATA_TYPE_LIST_TYPE buffer_size;
    uint32_t recevice_last_tick;
    uint8_t receive_finished : 1; // 1: receive finished
} udc_receive_t;


/*For future DMA adaptation, all data for a package is stored in a buffer*/
typedef struct _udc_transmit_t
{
    uint8_t *target_buf;
    uint8_t buffer[UDC_MEM_SIZE];
    UDC_DATA_TYPE_LIST_TYPE padding_size;
    UDC_DATA_TYPE_LIST_TYPE buffer_size;
} udc_transmit_t;


typedef struct 
{
    udc_event_dsc_t * event_dsc; /**< Dynamically allocated event callback and user data array*/

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
    #if UDC_ALL_USE_DEFAULT_DATA_TYPE==0
        UDC_DATA_TYPE_LIST_TYPE *data_type_list;      // data type list
        uint8_t                  data_type_list_size; // data type list size
    #endif
};





void udc_pack_init(udc_pack_t *pack);
void udc_pack_task(void);


int udc_set_data_type_list(udc_pack_t *pack, uint8_t *data_type_list, uint8_t data_type_list_size);
void udc_set_send_bytes_func(udc_pack_t *pack, udc_send_bytes_func_t send_bytes);
void udc_pack_receive_data(udc_pack_t *pack, const uint8_t *buf, UDC_DATA_TYPE_LIST_TYPE len);
int udc_append_data(udc_pack_t *pack, uint8_t id, uint8_t data_type, const void *data);
int udc_pack_push(udc_pack_t *pack);

UDC_DATA_TYPE_LIST_TYPE udc_get_padding_size(const udc_pack_t *pack, uint8_t revive_or_transmit);
UDC_DATA_TYPE_LIST_TYPE udc_get_data_type_size(const udc_pack_t *pack, uint8_t data_type);

udc_pack_t * udc_pack_get_header(void);
udc_pack_t * udc_pack_get_next(udc_pack_t * pack);

int udc_pack_get_obj(udc_pack_t *pack, uint8_t revive_or_transmit, uint8_t id, udc_obj_t *obj);
int udc_pack_get_first_obj(udc_pack_t *pack, uint8_t revive_or_transmit, udc_obj_t *obj);
int udc_pack_get_next_obj(udc_pack_t *pack, uint8_t revive_or_transmit, udc_obj_t *base_obj, udc_obj_t *next_obj);
bool udc_obj_is_end(const udc_pack_t *pack, uint8_t revive_or_transmit, udc_obj_t *obj);



#ifdef __cplusplus
}
#endif

#endif // __UDCHECK_H__
