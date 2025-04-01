#include "udc_core.h"
#include "stdlib.h"
#include "string.h"

#include "../misc/udc_mem.h"
#include "../hal/udc_hal_tick.h"

#if UDC_ALL_USE_DEFAULT_DATA_TYPE==1
    #ifdef UDC_DATA_TYPE_LIST_BUF
        static UDC_DATA_TYPE_LIST_TYPE *data_type_list_buf = UDC_DATA_TYPE_LIST_BUF;
        #if UDC_DATA_TYPE_LIZE_SIZE == -1
            static const uint8_t data_type_list_size = sizeof(UDC_DATA_TYPE_LIST_BUF);
        #else
            static const uint8_t data_type_list_size = UDC_DATA_TYPE_LIZE_SIZE;
        #endif
    #else
        static const UDC_DATA_TYPE_LIST_TYPE data_type_list_buf[] = {
            1, 2, 4, 8, 16, 32
        };
        static const uint8_t data_type_list_size = sizeof(data_type_list_buf)/sizeof(UDC_DATA_TYPE_LIST_TYPE);
    #endif
#endif

#define DATA_TYPE_SIZE    sizeof(UDC_DATA_TYPE_LIST_TYPE)
#define FIRST_DATA_OFFSET (DATA_TYPE_SIZE+UDC_PACKET_HEADERS_SIZE)

#define UDC_ABS(x) ((x) < 0 ? -(x) : (x) )



/**********************
 *  STATIC PROTOTYPES
 **********************/
static const UDC_DATA_TYPE_LIST_TYPE *get_data_type_list(const udc_pack_t *pack);
static uint8_t get_data_type_list_size(const udc_pack_t *pack);
static void set_padding_size(udc_pack_t *pack, uint8_t revive_or_transmit, UDC_DATA_TYPE_LIST_TYPE padding_size);
static udc_send_bytes_func_t get_send_bytes_func(const udc_pack_t *pack);
static uint8_t *get_target_buf(const udc_pack_t *pack, uint8_t revive_or_transmit);
static UDC_DATA_TYPE_LIST_TYPE get_target_buf_size(const udc_pack_t *pack, uint8_t revive_or_transmit);
static int get_pack_obj_for_data(udc_pack_t *pack, uint8_t *obj_data, udc_obj_t *obj);
static int get_pack_obj_for_buf(udc_pack_t *pack, uint8_t *obj_buf, udc_obj_t *obj);
static int get_transmit_pack_obj_for_new(udc_pack_t *pack, uint8_t id, uint8_t data_type, udc_obj_t *obj);
static int get_pack_verify(udc_pack_t *pack, uint8_t revice_or_transmit, uint8_t *verify);
static int check_receive_pack_header(udc_pack_t *pack);
/**********************
 *  STATIC VARIABLES
 **********************/

static udc_pack_t *udc_pack_head = NULL;



/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void udc_pack_init(udc_pack_t *pack)
{
    udc_memset_00(pack, sizeof(udc_pack_t));

    pack->receive.target_buf  = pack->receive.buffer;
    pack->receive.buffer_size = UDC_MEM_SIZE;

    pack->transmit.target_buf  = pack->transmit.buffer;
    pack->transmit.buffer_size = UDC_MEM_SIZE;

    pack->next = NULL;
    if (udc_pack_head == NULL)
    {
        udc_pack_head = pack;
    }
    else
    {
        udc_pack_t *temp = udc_pack_head;
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = pack;
    }
}


int udc_set_data_type_list(udc_pack_t *pack, uint8_t *data_type_list, uint8_t data_type_list_size)
{
    #if UDC_ALL_USE_DEFAULT_DATA_TYPE==0
        pack->data_type_list = data_type_list;
        pack->data_type_list_size = data_type_list_size;
        return 0;
    #endif
    return -1;
}



// 得到数据填充的大小
UDC_DATA_TYPE_LIST_TYPE udc_get_padding_size(const udc_pack_t *pack, uint8_t revive_or_transmit)
{
    switch (revive_or_transmit)
    {
        case 0:
            return pack->receive.padding_size;
            break;
        case 1:
            return pack->transmit.padding_size;
            break;
        default:
            return 0;
    }
    return 0;
}


udc_pack_t * udc_pack_get_header(void)
{
    return udc_pack_head;
}


udc_pack_t * udc_pack_get_next(udc_pack_t * pack)
{
    return pack->next;
}


// 得到类型的大小
UDC_DATA_TYPE_LIST_TYPE udc_get_data_type_size(const udc_pack_t *pack, uint8_t data_type)
{
    if (data_type >= get_data_type_list_size(pack)) return 0;
    const UDC_DATA_TYPE_LIST_TYPE *data_type_list = get_data_type_list(pack);
    if (NULL == data_type_list) return 0;
    return data_type_list[data_type];
}


// 判断obj在pack是不是最后一个
bool udc_obj_is_end(const udc_pack_t *pack, uint8_t revive_or_transmit, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj) return true;
    
    uint8_t *target_buf = get_target_buf(pack, revive_or_transmit);
    if (NULL == target_buf) return true;

    UDC_DATA_TYPE_LIST_TYPE padding_size = udc_get_padding_size(pack, revive_or_transmit);
    if (padding_size == 0) return true;

    UDC_DATA_TYPE_LIST_TYPE obj_end_pos = (uint32_t)obj->data - (uint32_t)target_buf + udc_get_data_type_size(pack, obj->data_type);
    if (UDC_ABS(padding_size-1 - obj_end_pos) <= 1) return true;

    return false;
}

// 得到pack中第一个obj
int udc_pack_get_first_obj(udc_pack_t *pack, uint8_t revive_or_transmit, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj) return -1;
    uint8_t *target_buf = get_target_buf(pack, revive_or_transmit);
    if (NULL == target_buf) return -1;
    if (udc_get_padding_size(pack, revive_or_transmit) < FIRST_DATA_OFFSET) return -1;

    if (0 != get_pack_obj_for_buf(pack, target_buf+FIRST_DATA_OFFSET, obj))
    {
        return -1;
    }
    return 0;
}

// 得到基于obj的下一个obj
int udc_pack_get_next_obj(udc_pack_t *pack, uint8_t revive_or_transmit, udc_obj_t *base_obj, udc_obj_t *next_obj)
{
    if (NULL == base_obj) return -1;
    if (udc_obj_is_end(pack, revive_or_transmit, base_obj)) return -1;
    if (0 != get_pack_obj_for_buf(pack, base_obj->data + udc_get_data_type_size(pack, base_obj->data_type), next_obj))
    {
        return -1;
    }

    return 0;
}

int udc_pack_get_obj(udc_pack_t *pack, uint8_t revive_or_transmit, uint8_t id, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj) return -1;
    udc_obj_t _obj;
    if (0 != udc_pack_get_first_obj(pack, revive_or_transmit, &_obj)) return -1;

    for (;;)
    {
        if (_obj.id == id)
        {
            obj->id = id;
            obj->data_type = _obj.data_type;
            obj->data = _obj.data;
            return 0;
        }
        if (0 != udc_pack_get_next_obj(pack, revive_or_transmit, &_obj, &_obj)) return -1;
    }
    return -1;
}


void udc_set_send_bytes_func(udc_pack_t *pack, udc_send_bytes_func_t send_bytes)
{
    pack->send_bytes = send_bytes;
}


int udc_append_data(udc_pack_t *pack, uint8_t id, uint8_t data_type, const void *data)
{
    if (NULL == pack || NULL == data) return -1;
    udc_obj_t obj;
    if (0 == udc_get_padding_size(pack, 1))
    {
        for (uint8_t i=0; i<UDC_PACKET_HEADERS_SIZE; i++)
        {
            pack->transmit.target_buf[i] = (uint8_t)UDC_PACKET_HEADERS_START[i];
        }
    }
    if (0 != get_transmit_pack_obj_for_new(pack, id, data_type, &obj))
    {
        return -1;
    }
    if (data_type != obj.data_type) return -1;

    obj.data[-2] = id;
    obj.data[-1] = data_type;
    memcpy(obj.data, data, udc_get_data_type_size(pack, data_type));
    return 0;
}

int udc_pack_push(udc_pack_t *pack)
{
    int ret = 0;
    udc_send_bytes_func_t send_bytes = get_send_bytes_func(pack);
    if (NULL == send_bytes) return -1;

    uint8_t* target_buf = get_target_buf(pack, 1);
    if (NULL == target_buf) return -1;

    UDC_DATA_TYPE_LIST_TYPE padding_size = udc_get_padding_size(pack, 1);
    UDC_DATA_TYPE_LIST_TYPE padding_verify_size = padding_size + UDC_INSPECT_BIT_SIZE;

    if (0 == padding_size) return -1;

    for (uint8_t i=UDC_PACKET_HEADERS_SIZE; i<FIRST_DATA_OFFSET; i++)
    {
        target_buf[i] = ( (padding_verify_size-FIRST_DATA_OFFSET)>>8 * (i-UDC_PACKET_HEADERS_SIZE) ) & 0xFF;
    }

    uint8_t verify = 0;
    if (0 != get_pack_verify(pack, 1, &verify)) return -1;
    target_buf[padding_size] = verify;
    set_padding_size(pack, 1, padding_verify_size);

    ret = send_bytes(target_buf, padding_verify_size);

    set_padding_size(pack, 1, 0);
    return ret;
}

void udc_pack_receive_data(udc_pack_t *pack, const uint8_t *buf, UDC_DATA_TYPE_LIST_TYPE len)
{
    volatile udc_receive_t * receive = &pack->receive;
    uint8_t * target_buf = get_target_buf(pack, 0);
    uint8_t * receive_size_target_buf = target_buf + UDC_PACKET_HEADERS_SIZE;

    receive->recevice_last_tick = udc_tick_get();

    if (1 == receive->receive_finished) return;
    if (udc_get_padding_size(pack, 0)+len >= get_target_buf_size(pack, 0))
    {
        set_padding_size(pack, 0, 0);
        return;
    }
    memcpy(target_buf+udc_get_padding_size(pack, 0), buf, len);
    set_padding_size(pack, 0, udc_get_padding_size(pack, 0)+len);
    if (udc_get_padding_size(pack, 0)>=FIRST_DATA_OFFSET && 0 != check_receive_pack_header(pack))
    {
        set_padding_size(pack, 0, 0);
        // 接收包头错误 （事件）
        return;
    }

    UDC_DATA_TYPE_LIST_TYPE receive_size = 0;
    for (uint8_t i=0; i<DATA_TYPE_SIZE; i++)
    {
        receive_size |= (UDC_DATA_TYPE_LIST_TYPE)receive_size_target_buf[i] << (8*i);
    }
    if (udc_get_padding_size(pack, 0) >= receive_size+FIRST_DATA_OFFSET)
    {
        receive->receive_finished = 1;
    }
}

void udc_pack_task(void)
{
    udc_pack_t * pack = udc_pack_get_header();
    uint8_t * receive_target_buf = get_target_buf(pack, 0);
    volatile udc_receive_t * receive = NULL;
    while (pack)
    {
        receive = &pack->receive;
        if (0==receive->receive_finished && udc_get_padding_size(pack, 0) && (udc_tick_elaps(receive->recevice_last_tick) >= UDC_PACK_RECEIVING_TIMEOUT) )
        {
            set_padding_size(pack, 0, 0);
            // 接收包间超时 （事件）
            udc_event_send_exe_now(pack, UDC_EVENT_RECEIVING_TIME_OUT, NULL);
            continue;
        }

        if (receive->receive_finished)
        {
            uint8_t verify = 0;
            if (get_pack_verify(pack, 0, &verify)==0 && receive_target_buf[udc_get_padding_size(pack, 0)-1] == verify)
            {
                // 接收完成 （事件）
                udc_event_send_exe_now(pack, UDC_EVENT_PACK_RECEIVE_FINSHED, NULL);
            }
            else
            {
                udc_event_send_exe_now(pack, UDC_EVENT_RECEIVE_FINSHED_VERIFR_ERROR, NULL);
                // 接收校验失败 （事件）
            }
            set_padding_size(pack, 0, 0);
            receive->receive_finished = 0;
        }
        pack = udc_pack_get_next(pack);
    }
}


/**********************
 *   STATIC FUNCTIONS
 **********************/


// 得到数据类型列表
static const UDC_DATA_TYPE_LIST_TYPE *get_data_type_list(const udc_pack_t *pack)
{
    #if UDC_ALL_USE_DEFAULT_DATA_TYPE==1
        return data_type_list_buf;
    #else
        return pack->data_type_list;
    #endif
}

// 得到数据类型列表大小
static uint8_t get_data_type_list_size(const udc_pack_t *pack)
{
    #if UDC_ALL_USE_DEFAULT_DATA_TYPE==1
        return data_type_list_size;
    #else
        return pack->data_type_list_size;
    #endif
}



static void set_padding_size(udc_pack_t *pack, uint8_t revive_or_transmit, UDC_DATA_TYPE_LIST_TYPE padding_size)
{
    switch (revive_or_transmit)
    {
        case 0:
            pack->receive.padding_size = padding_size;
            break;
        case 1:
            pack->transmit.padding_size = padding_size;
            break;
        default:
            break;
    }
}

static udc_send_bytes_func_t get_send_bytes_func(const udc_pack_t *pack)
{
    return pack->send_bytes;
}



// 得到目标缓冲区
static uint8_t *get_target_buf(const udc_pack_t *pack, uint8_t revive_or_transmit)
{
    switch (revive_or_transmit)
    {
        case 0:
            return pack->receive.target_buf;
            break;
        case 1:
            return pack->transmit.target_buf;
            break;
        default:
            return NULL;
    }
    return NULL;
}

// 得到目标缓冲区大小
static UDC_DATA_TYPE_LIST_TYPE get_target_buf_size(const udc_pack_t *pack, uint8_t revive_or_transmit)
{
    switch (revive_or_transmit)
    {
        case 0:
            return pack->receive.buffer_size;
            break;
        case 1:
            return pack->transmit.buffer_size;
            break;
        default:
            return 0;
    }
    return 0;
}

/**
 * @description: 在pack中获取obj
 * @param {udc_pack_t} *pack
 * @param {uint8_t} *obj_data 指向obj data的地址
 * @param {udc_obj_t} *obj
 * @return {*}
 */
static int get_pack_obj_for_data(udc_pack_t *pack, uint8_t *obj_data, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj_data || NULL == obj) return -1;

    obj->data = obj_data;
    obj->id = obj->data[-2];
    obj->data_type = obj->data[-1];
    return 0;
}

/**
 * @description: 在pack中获取obj
 * @param {udc_pack_t} *pack
 * @param {uint8_t} *obj_buf 指向obj 的存储头地址
 * @param {udc_obj_t} *obj
 * @return {*}
 */
static int get_pack_obj_for_buf(udc_pack_t *pack, uint8_t *obj_buf, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj_buf || NULL == obj) return -1;

    obj->data = obj_buf+2;
    obj->id = obj->data[-2];
    obj->data_type = obj->data[-1];
    return 0;
}



// 根据id获取obj，若没有则尝试创建一个新的obj
static int get_transmit_pack_obj_for_new(udc_pack_t *pack, uint8_t id, uint8_t data_type, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj) return -1;

    const UDC_DATA_TYPE_LIST_TYPE data_size = udc_get_data_type_size(pack, data_type);
    const UDC_DATA_TYPE_LIST_TYPE obj_size = 2 + data_size;
    uint8_t *target_buf = get_target_buf(pack, 1);
    const UDC_DATA_TYPE_LIST_TYPE target_buf_size = get_target_buf_size(pack, 1);

    if (0 == data_size || NULL == target_buf) return -1;

    if (0 == udc_get_padding_size(pack, 1)) 
    {
        obj->id = id;
        obj->data_type = data_type;
        obj->data = target_buf + FIRST_DATA_OFFSET + 2;
        set_padding_size(pack, 1, FIRST_DATA_OFFSET + obj_size);
    }
    else if (0 == udc_pack_get_obj(pack, 1, id, obj))
    {

    }
    else if (udc_get_padding_size(pack, 1) + obj_size + UDC_INSPECT_BIT_SIZE <= target_buf_size) 
    {
        obj->id = id;
        obj->data_type = data_type;
        obj->data = target_buf + udc_get_padding_size(pack, 1)+2;
        set_padding_size(pack, 1, udc_get_padding_size(pack, 1) + obj_size);
    }
    else 
    {
        return -1;
    }
    return 0;
}

// 得到pack的和校验值
static int get_pack_verify(udc_pack_t *pack, uint8_t revice_or_transmit, uint8_t *verify)
{
    uint8_t *target_buf = get_target_buf(pack, revice_or_transmit);
    if (NULL == target_buf) return -1;
    uint8_t _verify = 0;
    for (uint8_t i=0; i<FIRST_DATA_OFFSET; i++)
    {
        _verify += target_buf[i];
    }
    udc_obj_t obj;
    if (0 != udc_pack_get_first_obj(pack, revice_or_transmit, &obj)) return -1;
    for (;;)
    {
        _verify += obj.id;
        _verify += obj.data_type;

        for (UDC_DATA_TYPE_LIST_TYPE i=0; i<udc_get_data_type_size(pack, obj.data_type); i++)
        {
            _verify += obj.data[i];
        }
        if (0 != udc_pack_get_next_obj(pack, revice_or_transmit, &obj, &obj)) break;
    }
    *verify = _verify;
    return 0;
}

// 检查 receive pack 的头部是否符合标准
static int check_receive_pack_header(udc_pack_t *pack)
{
    uint8_t *target_buf = get_target_buf(pack, 0);
    for (uint8_t i=0; i<UDC_PACKET_HEADERS_SIZE; i++)
    {
        if (target_buf[i] != (uint8_t)UDC_PACKET_HEADERS_START[i])
        {
            return -1;
        } 
    }
    return 0;
}
