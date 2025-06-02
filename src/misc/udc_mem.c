#include "udc_mem.h"
#include "../../../udc_conf.h"
#if UDC_USE_ALLOC
    #include UDC_MEM_CUCTOM_INCLUDE
#endif




void udc_memset(void * dst, uint8_t v, size_t len)
{
    memset(dst, v, len);
}


void udc_memset_00(void * dst, size_t len)
{
    memset(dst, 0, len);
}


void * udc_mem_alloc(size_t size)
{
    #if UDC_USE_ALLOC
        return UDC_MEM_CUSTOM_ALLOC(size);
    #else
        return NULL;
    #endif
}

void udc_mem_free(void * ptr)
{
    #if UDC_USE_ALLOC
        UDC_MEM_CUSTOM_FREE(ptr);
    #endif
}

void * udc_mem_realloc(void * ptr, size_t size)
{
    #if UDC_USE_ALLOC
        return UDC_MEM_CUSTOM_REALLOC(ptr, size);
    #else
        return NULL;
    #endif
}


