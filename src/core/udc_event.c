#include "udc_core.h"
#include "../misc/udc_mem.h"


/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/
static int event_send_core(udc_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/



/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int udc_event_send_exe_now(struct _udc_pack_t * pack, udc_event_code_t event_code, void * param)
{
    if (NULL == pack) 
        return -1;

    udc_event_t e;
    e.target = pack;
    e.code = event_code;
    e.param = param;
    e.user_data = NULL;
    
    return event_send_core(&e);
}



udc_event_dsc_t * udc_pack_add_event_cb_static(struct _udc_pack_t * pack, udc_event_dsc_t  *event_dsc, udc_event_cb_t event_cb, udc_event_code_t filter,
                                                        void * user_data)
{
    if (NULL == event_dsc) {
        return NULL;
    }
    udc_memset_00(event_dsc, sizeof(udc_event_dsc_t));
    event_dsc->cb = event_cb;
    event_dsc->filter = filter;
    event_dsc->user_data = user_data;
    event_dsc->is_alloc = 0;
    event_dsc->next = NULL;


    udc_event_dsc_t *pack_event_dsc = pack->spec_attr.event_dsc;
    pack->spec_attr.event_dsc_cnt++;
    if (NULL == pack_event_dsc) {
        pack->spec_attr.event_dsc = event_dsc;
    }
    else {
        while (pack_event_dsc->next) {
            pack_event_dsc = pack_event_dsc->next;
        }
        pack_event_dsc->next = event_dsc;
    }

    return event_dsc;
}



struct _udc_pack_t * udc_event_get_target(udc_event_t * e)
{
    return e->target;
}


udc_event_code_t udc_event_get_code(udc_event_t * e)
{
    return e->code;
}

void * udc_event_get_param(udc_event_t * e)
{
   return e->param;
}


void * udc_event_get_user_data(udc_event_t * e)
{
    return e->user_data;
}




/**********************
 *   STATIC FUNCTIONS
 **********************/

static udc_event_dsc_t * udc_pack_get_event_dsc(const udc_pack_t * pack, uint32_t id)
{
    if (NULL == pack->spec_attr.event_dsc) return NULL;
    if(id >= pack->spec_attr.event_dsc_cnt) return NULL;

    return &pack->spec_attr.event_dsc[id];
}


static int event_send_core(udc_event_t * e)
{
    udc_event_dsc_t *event_dsc= udc_pack_get_event_dsc(e->target, 0);

    while (event_dsc) 
    {
        if (event_dsc->filter == e->code || event_dsc->filter == UDC_EVENT_ALL) 
        {
            e->user_data = event_dsc->user_data;
            event_dsc->cb(e);
        }
        event_dsc = event_dsc->next;
    }
    return 0;
}
