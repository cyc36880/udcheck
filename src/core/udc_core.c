#include "udc_core.h"
#include "stdlib.h"
#include "string.h"

#include "../misc/udc_mem.h"
#include "../hal/udc_hal_tick.h"

#define FIRST_DATA_OFFSET(pack) (pack->header.header_len + 2)

#define check_id(id) ( ( (id) >= 0 && (id) <= 119 ) || ( (id) >= 240 && (id) <= 255 ) )

#define id_to_objid(id, size) (id < 240 ?             \
                                    size <= 255 ?     \
                                        id : id + 120 \
                                : id)

#define objid_to_id(packid) (packid >= 120 ?              \
                                 packid < 240 ?           \
                                    packid - 120 : packid \
                              : packid)

#define obj_size_s(packid) (packid <= 119 ? 1 : 2)
#define obj_size_s2(id, size) (id < 240 ?     \
                                 size > 255 ? \
                                    2 : 1     \
                               : 2)

#define obj_header_size(packid) (1 + obj_size_s(packid))
#define obj_header_size2(id, size) (1 + obj_size_s2(id, size))

#define UDC_ABS(x) ( (x) < 0 ? -(x) : (x) )

/**********************
 *  STATIC PROTOTYPES
 **********************/
static uint16_t get_target_buffer_size(const udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit);
static int get_pack_obj_for_buf(const udc_pack_t *pack, uint8_t *obj_buf, udc_obj_t *obj);
static int check_receive_pack_header(udc_pack_t *pack);
static int get_transmit_pack_obj_for_new(udc_pack_t *pack, uint8_t id, uint16_t size, udc_obj_t *obj);
static int calculate_verify_value(const struct _udc_pack_t *pack, const uint8_t *data_buf, uint16_t buf_len, uint8_t *verify);
static void set_padding_size(udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit, uint16_t padding_size);
static udc_send_bytes_func_t get_send_bytes_func(const udc_pack_t *pack);
static void event_cb(udc_event_t * e);
/**********************
 *  STATIC VARIABLES
 **********************/
static udc_pack_t *udc_pack_head = NULL;
/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void udc_pack_init(udc_pack_init_t *pack_init)
{
    udc_pack_header_t header_temp = {
        .header = "\xFA",
        .header_len = 1
    };
    udc_pack_verify_t verify_temp = {
        .calculate_verify = calculate_verify_value,
        .verify_len = 1
    };
    udc_pack_t *pack = pack_init->pack;

    udc_memset_00(pack, sizeof(udc_pack_t));
    pack->header = pack_init->header;
    pack->verify = pack_init->verify;
    if (NULL == pack->header.header )
        pack->header = header_temp;
    if (NULL == pack->verify.calculate_verify)
        pack->verify = verify_temp;


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
    udc_pack_add_event_cb_static(pack, &pack->event, event_cb, UDC_EVENT_ALL, NULL);
}

void udc_pack_set_calculate_verify_func(udc_pack_t *pack, uint8_t verify_len, calculate_verify_func_t calculate_verify)
{
    pack->verify.verify_len = verify_len;
    pack->verify.calculate_verify = calculate_verify;
}

calculate_verify_func_t udc_pack_get_calculate_verify_func(const udc_pack_t *pack)
{
    return pack->verify.calculate_verify;
}

void udc_pack_set_send_bytes_func(udc_pack_t *pack, udc_send_bytes_func_t send_bytes)
{
    pack->send_bytes = send_bytes;
}

// 得到目标缓冲区
uint8_t *udc_pack_get_target_buffer(const udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit)
{
    switch (receive_or_transmit)
    {
    case 0:
        return pack->receive.target_buf;
//        break;
    case 1:
        return pack->transmit.target_buf;
//        break;
    default:
//        return NULL;
        break;
    }
    return NULL;
}

void *udc_pack_set_buffer_static(udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit, uint8_t *buffer, uint16_t buffer_size)
{
    udc_receive_t *receive = (receive_or_transmit == UDC_PACK_RECEIVE ? &pack->receive : (udc_receive_t *)&pack->transmit);
    receive->target_buf = buffer;
    receive->buffer_size = buffer_size;
    return receive;
}

// If the previous space was dynamically requested, using this function will not release it
int udc_pack_set_buffer(udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit, uint16_t buffer_size)
{
    void *alloc_buf = udc_mem_alloc(buffer_size);
    if (NULL == alloc_buf)
        return -1;
    udc_memset_00(alloc_buf, buffer_size);
    ((udc_receive_t *)udc_pack_set_buffer_static(pack, receive_or_transmit, alloc_buf, buffer_size))->target_buf_is_dynamic = 1;
    return 0;
}

// 得到数据填充的大小
uint16_t udc_pack_get_padding_size(const udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit)
{
    switch (receive_or_transmit)
    {
    case 0:
        return pack->receive.padding_size;
//        break;
    case 1:
        return pack->transmit.padding_size;
//        break;
    default:
//        return 0;
        break;
    }
    return 0;
}

udc_pack_t *udc_pack_get_header(void)
{
    return udc_pack_head;
}

udc_pack_t *udc_pack_get_next(const udc_pack_t *pack)
{
    return pack->next;
}

// 判断obj在pack是不是最后一个
bool udc_pack_obj_is_end(const udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj)
        return true;

    uint8_t *target_buf = udc_pack_get_target_buffer(pack, receive_or_transmit);
    if (NULL == target_buf)
        return true;

    uint16_t padding_size = udc_pack_get_padding_size(pack, receive_or_transmit);

    if (padding_size == 0)
        return true;
    uint16_t obj_end_pos = (uint32_t)obj->data - (uint32_t)target_buf + obj->size;
    if (UDC_ABS(padding_size - obj_end_pos) <= pack->verify.verify_len)
        return true;

    return false;
}

// 得到pack中第一个obj
int udc_pack_get_first_obj(const udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj)
        return -1;
    uint8_t *target_buf = udc_pack_get_target_buffer(pack, receive_or_transmit);
    if (NULL == target_buf)
        return -1;
    if (udc_pack_get_padding_size(pack, receive_or_transmit) < FIRST_DATA_OFFSET(pack))
        return -1;

    if (0 != get_pack_obj_for_buf(pack, target_buf + FIRST_DATA_OFFSET(pack), obj))
    {
        return -1;
    }
    return 0;
}

// 得到基于obj的下一个obj
int udc_pack_get_next_obj(const udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit, udc_obj_t *base_obj, udc_obj_t *next_obj)
{
    if (NULL == base_obj)
        return -1;
    if (udc_pack_obj_is_end(pack, receive_or_transmit, base_obj))
    {
        return -1;
    }
    if (0 != get_pack_obj_for_buf(pack, base_obj->data + base_obj->size, next_obj))
    {
        return -1;
    }
    return 0;
}

int udc_pack_get_obj(const udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit, uint8_t id, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj)
        return -1;
    udc_obj_t _obj;
    if (0 != udc_pack_get_first_obj(pack, receive_or_transmit, &_obj))
        return -1;

    UDC_PACK_OBJ_FOREACH(receive_or_transmit, pack, &_obj, if (_obj.id == id) {
            memcpy(obj, &_obj, sizeof(udc_obj_t));
            return 0; });

    return -1;
}

int udc_pack_append_data(udc_pack_t *pack, uint8_t id, uint16_t size, const void *data)
{
    if (!check_id(id))
        return -1;
    if (NULL == pack || (size > 0 && NULL == data))
        return -1;
    udc_obj_t obj;
    if (NULL == udc_pack_get_target_buffer(pack, UDC_PACK_TRANSMIT)) 
        return -1;
    if (0 == udc_pack_get_padding_size(pack, UDC_PACK_TRANSMIT))
    {
        for (uint8_t i = 0; i < pack->header.header_len; i++)
        {
            pack->transmit.target_buf[i] = (uint8_t)pack->header.header[i];
        }
    }
    if (0 != get_transmit_pack_obj_for_new(pack, id, size, &obj))
    {
        return -1;
    }
    uint8_t i = 0;
    for (; i < obj_size_s2(id, size); i++)
    {
        obj.data[-(i + 1)] = (obj.size >> (i * 8)) & 0xFF;
    }
    obj.data[-(i + 1)] = obj.obj_id;
    memcpy(obj.data, data, size);
    return 0;
}

int udc_pack_push(udc_pack_t *pack)
{
    int ret = 0;
    udc_send_bytes_func_t send_bytes = get_send_bytes_func(pack);
    if (NULL == send_bytes)
        return -1;

    uint8_t *target_buf = udc_pack_get_target_buffer(pack, UDC_PACK_TRANSMIT);
    if (NULL == target_buf)
        return -1;

    calculate_verify_func_t calculate_verify = udc_pack_get_calculate_verify_func(pack);
    if (NULL == calculate_verify)
        return -1;

    uint16_t padding_size = udc_pack_get_padding_size(pack, UDC_PACK_TRANSMIT);
    const uint16_t padding_verify_size = padding_size + pack->verify.verify_len;
    const uint16_t pack_size = padding_verify_size - FIRST_DATA_OFFSET(pack);

    if (0 == padding_size)
        return -1;

    target_buf[pack->header.header_len + 0] = (pack_size >> 8) & 0xFF;
    target_buf[pack->header.header_len + 1] = (pack_size) & 0xFF;

    uint8_t verify[pack->verify.verify_len];
    if (0 != calculate_verify(pack, target_buf, padding_size, verify))
        return -1;
    for (uint8_t i = 0; i < pack->verify.verify_len; i++, padding_size++)
    {
        target_buf[padding_size] = verify[i];
    }
    padding_size--;
    set_padding_size(pack, UDC_PACK_TRANSMIT, padding_verify_size);

    ret = send_bytes(pack, target_buf, padding_verify_size);

    set_padding_size(pack, UDC_PACK_TRANSMIT, 0);
    return ret;
}

void udc_pack_receive_data(udc_pack_t *pack, const uint8_t *buf, uint16_t len)
{
    volatile udc_receive_t *receive = &pack->receive;
    if (1 == receive->receive_finished)
        return;

    uint8_t *target_buf = udc_pack_get_target_buffer(pack, UDC_PACK_RECEIVE);
    uint8_t *receive_size_target_buf = target_buf + pack->header.header_len;

    receive->recevice_last_tick = udc_tick_get();

    if (udc_pack_get_padding_size(pack, UDC_PACK_RECEIVE) + len >= get_target_buffer_size(pack, UDC_PACK_RECEIVE))
    {
        // UDC_EVENT_RECEIVE_PADDING_OUT
        // udc_event_send_exe_now(pack, UDC_EVENT_RECEIVE_PADDING_OUT, NULL);
        receive->check_receive_heard_flag = 0;
        set_padding_size(pack, UDC_PACK_RECEIVE, 0);
        return;
    }
    memcpy(target_buf + udc_pack_get_padding_size(pack, UDC_PACK_RECEIVE), buf, len);
    set_padding_size(pack, UDC_PACK_RECEIVE, udc_pack_get_padding_size(pack, UDC_PACK_RECEIVE) + len);
    if (receive->check_receive_heard_flag==0 && udc_pack_get_padding_size(pack, UDC_PACK_RECEIVE) >= FIRST_DATA_OFFSET(pack))
    {
        if (0 != check_receive_pack_header(pack))
        {
            // UDC_EVENT_RECEIVE_HEADER_ERROR
            // udc_event_send_exe_now(pack, UDC_EVENT_RECEIVE_HEADER_ERROR, NULL);
            receive->check_receive_heard_flag = 0;
            set_padding_size(pack, UDC_PACK_RECEIVE, 0);
            return;
        }
        uint16_t receive_size = 0;
        receive_size = receive_size_target_buf[0];
        receive_size <<= 8;
        receive_size |= receive_size_target_buf[1];
        receive->ready_receive_data_size = receive_size;
        receive->check_receive_heard_flag = 1;
    }
    
    if (udc_pack_get_padding_size(pack, UDC_PACK_RECEIVE) >= receive->ready_receive_data_size + FIRST_DATA_OFFSET(pack))
    {
        receive->receive_finished = 1;
    }
}

int udc_pack_push_single(udc_pack_t *pack, uint8_t id, uint16_t size, const void *data)
{
    const uint16_t pack_size = FIRST_DATA_OFFSET(pack) + obj_header_size2(id, size) + size + pack->verify.verify_len;
    uint8_t buffer[pack_size];

    udc_transmit_t *transmit = &pack->transmit;
    udc_transmit_t tr_tmp = pack->transmit;
    udc_memset_00(transmit, sizeof(udc_transmit_t));
    
    transmit->target_buf = buffer;
    transmit->buffer_size = pack_size;

    int ret = udc_pack_append_data(pack, id, size, data);
    ret += udc_pack_push(pack);

    pack->transmit = tr_tmp;

    return ret;
}

int udc_pack_check_receive_verify(udc_pack_t *pack)
{
    uint8_t verify[pack->verify.verify_len];
    uint8_t *verify_buf = NULL;
    uint16_t rev_size = udc_pack_get_padding_size(pack, UDC_PACK_RECEIVE);
    uint8_t *target_buf = udc_pack_get_target_buffer(pack, UDC_PACK_RECEIVE);
    calculate_verify_func_t calculate_verify = udc_pack_get_calculate_verify_func(pack);
    if (NULL == calculate_verify)
        return -1;
    if (0 != calculate_verify(pack, target_buf, rev_size - pack->verify.verify_len, verify))
        return -1;
    verify_buf = target_buf + udc_pack_get_padding_size(pack, UDC_PACK_RECEIVE) - pack->verify.verify_len;
    for (uint8_t i = 0; i < pack->verify.verify_len; i++)
    {
        if (verify_buf[i] != verify[i])
            return -1;
    }
    return 0;
}

void udc_pack_task(void)
{
    udc_pack_t *pack = udc_pack_get_header();
    uint8_t *receive_target_buf = udc_pack_get_target_buffer(pack, UDC_PACK_RECEIVE);
    volatile udc_receive_t *receive = NULL;
    while (pack)
    {
        int pack_push_ret = udc_pack_push(pack);
        if (pack_push_ret > 0)
        {
            udc_event_send_exe_now(pack, UDC_EVENT_PACK_TRANSMIT_FINSHED, (void *)&pack_push_ret);
        }
        receive = &pack->receive;
        if (0 == receive->receive_finished && udc_pack_get_padding_size(pack, UDC_PACK_RECEIVE) && (udc_tick_elaps(receive->recevice_last_tick) >= UDC_PACK_RECEIVING_TIMEOUT))
        {
            if (udc_pack_get_padding_size(pack, UDC_PACK_RECEIVE) >= FIRST_DATA_OFFSET(pack))
            {
                // 接收包间超时 （事件）
                udc_event_send_exe_now(pack, UDC_EVENT_RECEIVING_TIME_OUT, NULL);
            }
            set_padding_size(pack, UDC_PACK_RECEIVE, 0);
            continue;
        }

        if (receive->receive_finished)
        {
            if (0 == udc_pack_check_receive_verify(pack))
            {
                // 接收完成 （事件）
                udc_event_send_exe_now(pack, UDC_EVENT_PACK_RECEIVE_FINSHED, NULL);
            }
            else
            {
                // 接收校验失败 （事件）
                udc_event_send_exe_now(pack, UDC_EVENT_RECEIVE_FINSHED_VERIFY_ERROR, NULL);
            }
            set_padding_size(pack, UDC_PACK_RECEIVE, 0);
            receive->receive_finished = 0;
        }
        pack = udc_pack_get_next(pack);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void set_padding_size(udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit, uint16_t padding_size)
{
    switch (receive_or_transmit)
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

/**
 * @description: 在pack中获取obj
 * @param {udc_pack_t} *pack
 * @param {uint8_t} *obj_buf 指向obj 的存储头地址
 * @param {udc_obj_t} *obj
 * @return {*}
 */
static int get_pack_obj_for_buf(const udc_pack_t *pack, uint8_t *obj_buf, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj_buf || NULL == obj)
        return -1;

    obj->obj_id = obj_buf[0];
    obj->id = objid_to_id(obj->obj_id);
    obj->data = obj_buf + obj_header_size(obj->obj_id);
    obj->size = 0;
    for (uint8_t i = 0; i < obj_size_s(obj->obj_id); i++)
    {
        obj->size <<= 8;
        obj->size |= obj_buf[1 + i];
    }
    return 0;
}

// 得到目标缓冲区大小
static uint16_t get_target_buffer_size(const udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit)
{
    switch (receive_or_transmit)
    {
    case 0:
        return pack->receive.buffer_size;
//        break;
    case 1:
        return pack->transmit.buffer_size;
//        break;
    default:
//        return 0;
        break;
    }
    return 0;
}

// 根据id获取obj，若没有则尝试创建一个新的obj
static int get_transmit_pack_obj_for_new(udc_pack_t *pack, uint8_t id, uint16_t size, udc_obj_t *obj)
{
    if (NULL == pack || NULL == obj)
        return -1;

    uint8_t *target_buf = udc_pack_get_target_buffer(pack, UDC_PACK_TRANSMIT);
    const uint16_t target_buf_size = get_target_buffer_size(pack, UDC_PACK_TRANSMIT);
    const uint16_t obj_header_size = obj_header_size2(id, size);
    const uint16_t obj_size = obj_header_size + size;

    if (NULL == target_buf)
        return -1;

    if (0 == udc_pack_get_padding_size(pack, UDC_PACK_TRANSMIT))
    {
        obj->id = id;
        obj->obj_id = id_to_objid(obj->id, size);
        obj->size = size;
        obj->data = target_buf + FIRST_DATA_OFFSET(pack) + obj_header_size;
        set_padding_size(pack, UDC_PACK_TRANSMIT, FIRST_DATA_OFFSET(pack) + obj_size);
    }
    else if (0 == udc_pack_get_obj(pack, UDC_PACK_TRANSMIT, id, obj))
    {
        if (size != obj->size)
            return -1;
    }
    else if (udc_pack_get_padding_size(pack, UDC_PACK_TRANSMIT) + obj_size + pack->verify.verify_len <= target_buf_size)
    {
        obj->id = id;
        obj->obj_id = id_to_objid(id, size);
        obj->size = size;
        obj->data = target_buf + udc_pack_get_padding_size(pack, UDC_PACK_TRANSMIT) + obj_header_size;
        set_padding_size(pack, UDC_PACK_TRANSMIT, udc_pack_get_padding_size(pack, UDC_PACK_TRANSMIT) + obj_size);
    }
    else
    {
        return -1;
    }
    return 0;
}

// 得到pack的和校验值
static int calculate_verify_value(const struct _udc_pack_t *pack, const uint8_t *data_buf, uint16_t buf_len, uint8_t *verify)
{
    if (NULL == data_buf)
        return -1;
    uint8_t _verify = 0;
    for (uint16_t i = 0; i < buf_len; i++)
    {
        _verify += data_buf[i];
    }
    verify[0] = _verify;
    return 0;
}


// 检查 receive pack 的头部是否符合标准
static int check_receive_pack_header(udc_pack_t *pack)
{
    uint8_t *target_buf = udc_pack_get_target_buffer(pack, UDC_PACK_RECEIVE);
    for (uint8_t i = 0; i < pack->header.header_len; i++)
    {
        if (target_buf[i] != (uint8_t)pack->header.header[i])
        {
            return -1;
        }
    }
    return 0;
}

// =============== event_cb ===================
static void event_cb(udc_event_t * e)
{
    udc_event_code_t code = udc_event_get_code(e);
    udc_pack_t *pack = udc_event_get_target(e);
    udc_receive_t *receive = &pack->receive;
    switch (code)
    {
        case UDC_EVENT_RECEIVING_TIME_OUT:
        case UDC_EVENT_RECEIVE_PADDING_OUT:
        case UDC_EVENT_RECEIVE_HEADER_ERROR:
        case UDC_EVENT_RECEIVE_FINSHED_VERIFY_ERROR:
        case UDC_EVENT_PACK_RECEIVE_FINSHED:
            receive->check_receive_heard_flag = 0;
        case UDC_EVENT_PACK_TRANSMIT_FINSHED:
        default: break;
    }
}


