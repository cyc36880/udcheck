#ifndef __UDC_CORE_H__
#define __UDC_CORE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../../../udc_conf.h"
#include "stdbool.h"
#include "udc_event.h"

struct _udc_pack_t;

// return lenth of data
typedef int (*udc_send_bytes_func_t)(const struct _udc_pack_t *pack, const uint8_t *buf, uint16_t len);
typedef int (*calculate_verify_func_t)(const struct _udc_pack_t *pack, const uint8_t *buf, uint16_t len, uint8_t *verify);

#define UDC_PACK_OBJ_FOREACH(recive_or_transmit, pack, obj, active)               \
    do                                                                            \
    {                                                                             \
        if (0 != udc_pack_get_first_obj(pack, recive_or_transmit, obj)) break;    \
        do                                                                        \
        {                                                                         \
            active                                                                \
        } while (0 == udc_pack_get_next_obj(pack, recive_or_transmit, obj, obj)); \
    } while (0)


#define UDC_PACK_RECEIVE_WAIT(pack, buffer, buf_len, timeout, active) \
    do                                                                \
    {                                                                 \
        udc_receive_t *pack_rev = &(pack)->receive;                   \
        udc_receive_t rev_tmp = (pack)->receive;                      \
        udc_memset_00(pack_rev, sizeof(udc_receive_t));               \
        pack_rev->target_buf = (buffer);                              \
        pack_rev->buffer_size = (buf_len);                            \
        uint32_t last_tick = udc_tick_get();                          \
        uint8_t active_flag = 1;                                      \
        while (pack_rev->receive_finished == 0)                       \
        {                                                             \
            if (udc_tick_elaps(last_tick) > (timeout))                \
            {                                                         \
                active_flag = 0;                                      \
                break;                                                \
            }                                                         \
        }                                                             \
        if (active_flag && udc_pack_check_receive_verify(pack)==0)    \
        {                                                             \
            active                                                    \
        }                                                             \
        (pack)->receive = rev_tmp;                                    \
    } while (0)

typedef enum 
{
    UDC_PACK_RECEIVE = 0,
    UDC_PACK_TRANSMIT = 1,
} udc_pack_receive_or_transmit_t;

typedef struct _udc_obj_t
{
    uint8_t *data;
    uint16_t size;   // data len
    uint8_t id;      // data id
    uint8_t pack_id; // pack_id=id+120 when size>255, otherwise pack_id=id
} udc_obj_t;

typedef struct _udc_receive_t
{
    uint8_t *target_buf;
    uint16_t padding_size;
    uint16_t buffer_size;
    uint8_t target_buf_is_dynamic : 1;     //  Whether the buffer is dynamically applied
    uint8_t receive_finished : 1;          //  receive finished
    uint8_t check_receive_heard_flag : 1;  //  check receive heard flag
    uint16_t ready_receive_data_size;      //  ready receive data size
    uint32_t recevice_last_tick;
} udc_receive_t;

/*For future DMA adaptation, all data for a package is stored in a buffer*/
typedef struct _udc_transmit_t
{
    uint8_t *target_buf;
    uint16_t padding_size;
    uint16_t buffer_size;
    uint8_t target_buf_is_dynamic : 1; //  Whether the buffer is dynamically applied
} udc_transmit_t;

typedef struct _udc_pack_header_t
{
    const char * header;
    uint8_t header_len;
} udc_pack_header_t;

typedef struct _udc_pack_verify_t
{
    calculate_verify_func_t calculate_verify;
    uint8_t verify_len;
} udc_pack_verify_t;

typedef struct
{
    udc_event_dsc_t *event_dsc;

    uint8_t event_dsc_cnt;
} _udc_obj_spec_attr_t;

typedef struct _udc_pack_t udc_pack_t;
struct _udc_pack_t
{
    udc_pack_t *next;
    udc_send_bytes_func_t send_bytes;
    _udc_obj_spec_attr_t spec_attr;
    udc_event_dsc_t event;
    udc_receive_t receive;
    udc_transmit_t transmit;
    udc_pack_header_t header;
    udc_pack_verify_t verify;
};

typedef struct _udc_pack_init
{
    udc_pack_t *pack;
    udc_pack_header_t header;
    udc_pack_verify_t verify;
} udc_pack_init_t;

/**
 * @description: Initialize the package.
 * @param pack_init Initialization parameters
 * @return {*}
 */
void udc_pack_init(udc_pack_init_t *pack_init);

/**
 * @description: The receive function can be used in interrupts.
 * @param pack Package pointer
 * @param buf  reveive buffer pointer
 * @param len  reveive buffer length
 * @return {*}
 */
void udc_pack_receive_data(udc_pack_t *pack, const uint8_t *buf, uint16_t len);

/**
 * @description: udcheck run the function.
 * @return {*}
 */
void udc_pack_task(void);

/**
 * @description: Set up the sending function.
 * @param pack       Package pointer
 * @param send_bytes Sending function
 * @return {*}
 */
void udc_pack_set_send_bytes_func(udc_pack_t *pack, udc_send_bytes_func_t send_bytes);

/**
 * @description: Set up a function for calculating the check value.
 * @param pack             Package pointer
 * @param verify_len       verify length
 * @param calculate_verify Calculate the check value function
 * @return {*}
 */
void udc_pack_set_calculate_verify_func(udc_pack_t *pack, uint8_t verify_len, calculate_verify_func_t calculate_verify);

/**
 * @description: Get the function for calculating the check value.
 * @param pack Package pointer
 * @return     calculate_verify_func_t type of function
 */
calculate_verify_func_t udc_pack_get_calculate_verify_func(const udc_pack_t *pack);

/**
 * @description: Get the buffer.
 * @param pack                Package pointer
 * @param receive_or_transmit see udc_pack_receive_or_transmit_t
 * @return                    buffer
 */
uint8_t *udc_pack_get_target_buffer(const udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit);

/**
 * @description: The checksum value of the received packet is calculated and compared with the checksum value in the packet.
 * @param pack Package pointer
 * @return     0:success -1:failed
 */
int udc_pack_check_receive_verify(udc_pack_t *pack);

/**
 * @description: Set the buffer.
 *  @note example : ((udc_receive_t *)udc_pack_set_buffer_static(pack, receive_or_transmit, buffer, buffer_size))->target_buf_is_dynamic = 0.
 * @param pack                Package pointer
 * @param receive_or_transmit   see udc_pack_receive_or_transmit_t
 * @param buffer                buffer pointer
 * @param buffer_size           buffer size
 * @return                      Returns the action handle corresponding to the "receive_or_transmit"
 */
void *udc_pack_set_buffer_static(udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit, uint8_t *buffer, uint16_t buffer_size);

/**
 * @description: Set up a dynamic request buffer.
 *  @note If the previous space was dynamically requested, using this function will not release it.
 * @param pack                  Package pointer
 * @param receive_or_transmit   see udc_pack_receive_or_transmit_t
 * @param  buffer_size          buffer size
 * @return                      0:success -1:failed
 */
int udc_pack_set_buffer(udc_pack_t *pack, udc_pack_receive_or_transmit_t receive_or_transmit, uint16_t buffer_size);

/**
 * @description: Add an obj to the staging area.
 *  @note If there is an obj with the same ID in the staging area, and the len has always been,
 *        it will be replaced; If only the id is the same but the len is inconsistent, the addition fails.
 * @param pack  Package pointer
 * @param id    obj id
 * @param size  obj size
 * @param data  obj data
 * @return      0:success -1:faile
 */
int udc_pack_append_data(udc_pack_t *pack, uint8_t id, uint16_t size, const void *data);

/**
 * @description: Package the data in the staging area and send the data (if any).
 *  @note When there is data in the buffer, the "udc_pack_task" actively calls this function.
 * @param pack Package pointer
 * @return     0:success -1:failed
 */
int udc_pack_push(udc_pack_t *pack);

/**
 * @description: Push a single obj out directly, and the stack space used by the cache should not be too large
 *  @note The cache comes from the stack and does not trigger 'UDC_EVENT_PACK_TRANSMIT_FINSHED' events
 * @param pack  Package pointer.
 * @param id    obj id
 * @param size  obj size
 * @param data  obj data
 * @return      0:success -1:failed
 */
int udc_pack_push_single(udc_pack_t *pack, uint8_t id, uint16_t size, const void *data);

/**
 * @description: Gets the size of the buffer staging data.
 * @param pack               Package pointer
 * @param revive_or_transmit 0:receive 1:transmit
 * @return                   size
 */
uint16_t udc_pack_get_padding_size(const udc_pack_t *pack, udc_pack_receive_or_transmit_t revive_or_transmit);

/**
 * @description: Get the head node of the pack.
 * @return Head node pointer
 */
udc_pack_t *udc_pack_get_header(void);

/**
 * @description: Get the next node based on the current node.
 * @param pack Current node pointer
 * @return    Next node pointer
 */
udc_pack_t *udc_pack_get_next(const udc_pack_t *pack);

/**
 * @description: Determine whether the current obj is the last in the buffer.
 * @param pack               Package pointer
 * @param revive_or_transmit 0:receive 1:transmit
 * @param obj                obj pointer
 * @return                   1:yes 0:no
 */
bool udc_pack_obj_is_end(const udc_pack_t *pack, udc_pack_receive_or_transmit_t revive_or_transmit, udc_obj_t *obj);

/**
 * @description: Determine whether the current obj is the last in the buffer.
 * @param pack               Package pointer
 * @param revive_or_transmit 0:receive 1:transmit
 * @param obj                obj pointer
 * @return                   0:success -1:failed
 */
int udc_pack_get_first_obj(const udc_pack_t *pack, udc_pack_receive_or_transmit_t revive_or_transmit, udc_obj_t *obj);

/**
 * @description: Get the next obj node based on the current obj
 * @param pack               Package pointer
 * @param revive_or_transmit 0:receive 1:transmit
 * @param base_obj           Current obj pointer
 * @param next_obj           Next obj pointer
 * @return                   0:success -1:failed
 */
int udc_pack_get_next_obj(const udc_pack_t *pack, udc_pack_receive_or_transmit_t revive_or_transmit, udc_obj_t *base_obj, udc_obj_t *next_obj);

/**
 * @description: Get the obj based on the id in the buffer
 * @param pack               Package pointer
 * @param revive_or_transmit 0:receive 1:transmit
 * @param id                 obj id
 * @param obj                obj pointer
 * @return                   0:success -1:failed
 */
int udc_pack_get_obj(const udc_pack_t *pack, udc_pack_receive_or_transmit_t revive_or_transmit, uint8_t id, udc_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif // __UDCHECK_H__
