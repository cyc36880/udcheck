#ifndef __UDC_HAL_TICK_H__
#define __UDC_HAL_TICK_H__

#ifdef __cplusplus
extern "C" {
#endif    

#include "udc_conf.h"



#if UDC_TICK_CUSTOM==0
/**
 * You have to call this function periodically
 * must be in 1 millisecond cycles
 */
void udc_tick_inc(uint32_t ms);
#endif

/**
 * Get the elapsed milliseconds since start up
 * @return the elapsed milliseconds
 */
uint32_t udc_tick_get(void);

/**
 * Get the elapsed milliseconds since a previous time stamp
 * @param prev_tick a previous time stamp (return value of udc_tick_get() )
 * @return the elapsed milliseconds since 'prev_tick'
 */
uint32_t udc_tick_elaps(uint32_t prev_tick);

#ifdef __cplusplus
}
#endif

#endif // __UDC_HAL_TICK_H__
