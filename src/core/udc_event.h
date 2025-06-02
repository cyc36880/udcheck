#ifndef __UDC_EVENT_H__
#define __UDC_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/**********************
 *      TYPEDEFS
 **********************/

struct _udc_pack_t;
struct _udc_event_dsc_t;


/**
 * Type of event being sent to the object.
 */
typedef enum 
{
    UDC_EVENT_ALL = 0,
    UDC_EVENT_RECEIVING_TIME_OUT,     /* receiving time out*/
    UDC_EVENT_RECEIVE_PADDING_OUT,    /* receive padding out, only user_alloc*/
    UDC_EVENT_RECEIVE_HEADER_ERROR,   /* receive header error, only user_alloc*/
    UDC_EVENT_RECEIVE_FINSHED_VERIFY_ERROR,   /* receive finsh but verify error*/
    
    UDC_EVENT_PACK_RECEIVE_FINSHED,   /* receice finsh */
    UDC_EVENT_PACK_TRANSMIT_FINSHED,  /* transmit finsh, int -> param*/ 

    _UDC_EVENT_LAST,  /** Number of default events*/

} udc_event_code_t;

typedef struct _udc_event_t 
{
    struct _udc_pack_t *target;
    udc_event_code_t code;
    void * user_data;
    void * param;
} udc_event_t;


/**
 * @brief Event callback.
 * Events are used to notify the user of some action being taken on the object.
 * For details, see ::udc_event_t.
 */
typedef void (*udc_event_cb_t)(udc_event_t * e);



typedef struct _udc_event_dsc_t 
{
    udc_event_cb_t cb;
    void * user_data;
    struct _udc_event_dsc_t * next;
    udc_event_code_t filter;
    uint8_t is_alloc : 1; // 0 no alloc, 1 alloc
} udc_event_dsc_t;







/**********************
 * GLOBAL PROTOTYPES
 **********************/


int udc_event_send_exe_now(struct _udc_pack_t * pack, udc_event_code_t event_code, void * param);

/**
 * Get the object originally targeted by the event. It's the same even if the event is bubbled.
 * @param e     pointer to the event descriptor
 * @return      the target of the event_code
 */
struct _udc_pack_t * udc_event_get_target(udc_event_t * e);

/**
 * Get the event code of an event
 * @param e     pointer to the event descriptor
 * @return      the event code.
 */
udc_event_code_t udc_event_get_code(udc_event_t * e);

/**
 * Get the parameter passed when the event was sent
 * @param e     pointer to the event descriptor
 * @return      pointer to the parameter
 */
void * udc_event_get_param(udc_event_t * e);

/**
 * Get the user_data passed when the event was registered on the object
 * @param e     pointer to the event descriptor
 * @return      pointer to the user_data
 */
void * udc_event_get_user_data(udc_event_t * e);

/**
 * Register a new, custom event ID.
 * It can be used the same way as e.g. `UDC_EVENT_PACK_TRANSMIT_FINSHED` to send custom events
 * @return     the new event id
 */
uint32_t udc_event_register_id(void);


/**
 * Add an event handler function for an object.
 * Used by the user to react on event which happens with the object.
 * An object can have multiple event handler. They will be called in the same order as they were added.
 * @param pack      pointer to an pack
 * @param event_dsc pointer to an event descriptor(staticly allocated)
 * @param filter    and event code
 * @param event_cb  the new event function
 * @param user_data custom data data will be available in `event_cb`
 * @return          a pointer the event descriptor. 
 */
udc_event_dsc_t * udc_pack_add_event_cb_static(struct _udc_pack_t * pack, udc_event_dsc_t  *event_dsc, udc_event_cb_t event_cb, 
                                                udc_event_code_t filter, void * user_data);

/**
 * Add an event handler function for an object.
 * Used by the user to react on event which happens with the object.
 * An object can have multiple event handler. They will be called in the same order as they were added.
 * @param pack      pointer to an pack
 * @param filter    and event code
 * @param event_cb  the new event function
 * @param user_data custom data data will be available in `event_cb`
 * @return          a pointer the event descriptor. 
 */
udc_event_dsc_t * udc_pack_add_event_cb(struct _udc_pack_t * pack, udc_event_cb_t event_cb, udc_event_code_t filter, void * user_data);

 
/**
 * Remove an event handler function for an object.
 * @param pack      pointer to an pack
 * @param event_cb  the event function to remove, or `NULL` to remove the firstly added event callback
 * @return          true if any event handlers were removed
 */
int udc_pack_remove_event_cb(struct _udc_pack_t * pack, udc_event_cb_t event_cb);

#ifdef __cplusplus
}
#endif

#endif
