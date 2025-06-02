#ifndef __UDC_MEM_H__
#define __UDC_MEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/**
 * Same as `memset` but optimized for 4 byte operation.
 * @param dst pointer to the destination buffer
 * @param v value to set [0..255]
 * @param len number of byte to set
 */
void udc_memset(void * dst, uint8_t v, size_t len);

/**
 * Same as `memset(dst, 0x00, len)` but optimized for 4 byte operation.
 * @param dst pointer to the destination buffer
 * @param len number of byte to set
 */
void udc_memset_00(void * dst, size_t len);

void * udc_mem_alloc(size_t size);
void udc_mem_free(void * ptr);
void * udc_mem_realloc(void * ptr, size_t size);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __UDC_MEM_H__ */
