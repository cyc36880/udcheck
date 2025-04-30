#include "udc_mem.h"
#include "../../../udc_conf.h"
#include UDC_MEM_CUCTOM_INCLUDE




void udc_memset(void * dst, uint8_t v, size_t len)
{
    memset(dst, v, len);
}


void udc_memset_00(void * dst, size_t len)
{
    memset(dst, 0, len);
}


void * udc_alloc(size_t size)
{
    return UDC_MEM_CUSTOM_ALLOC(size);
}

void udc_free(void * ptr)
{
    UDC_MEM_CUSTOM_FREE(ptr);
}

void * udc_realloc(void * ptr, size_t size)
{
    return UDC_MEM_CUSTOM_REALLOC(ptr, size);
}


