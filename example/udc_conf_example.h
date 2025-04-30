#ifndef __UDC_CONF_H__
#define __UDC_CONF_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/*The buffer in which the data is received,
 * Send and receive separate buffers.*/
#define UDC_MEM_SIZE 200

// Packet headers
#define UDC_PACKET_HEADERS_SIZE  1
#define UDC_PACKET_HEADERS_START "\xFA"

// Inspect the bits
#define UDC_INSPECT_BIT_SIZE     1

#define UDC_PACK_RECEIVING_TIMEOUT  20 /*ms*/

#define UDC_USE_ALLOC 0
#if UDC_USE_ALLOC == 0
    #define UDC_MEM_CUCTOM_INCLUDE           "stdlib.h"
    #define UDC_MEM_CUSTOM_ALLOC(x)           malloc(x)
    #define UDC_MEM_CUSTOM_FREE(x)            free(x)
    #define UDC_MEM_CUSTOM_REALLOC(ptr, size) realloc(ptr, size)
#else
    #define UDC_MEM_CUCTOM_INCLUDE           "Arduino.h"
    #define UDC_MEM_CUSTOM_ALLOC(x)           malloc(x)
    #define UDC_MEM_CUSTOM_FREE(x)            free(x)
    #define UDC_MEM_CUSTOM_REALLOC(ptr, size) realloc(ptr, size)
#endif

/*Use a custom tick source that tells the elapsed time in milliseconds.
 *It removes the need to manually update the tick with `udc_tick_inc()`).
*/
#define UDC_TICK_CUSTOM 0
#if UDC_TICK_CUSTOM==1
    #define UDC_TICK_CUSTOM_INCLUDE "Arduino.h"
    #define UDC_TICK_CUSTOM_SYS_TIME_ESPR    (millis())
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __UDC_CONF_H__
