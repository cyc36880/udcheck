#include "udc_mem.h"




void udc_memset(void * dst, uint8_t v, size_t len)
{
    memset(dst, v, len);
}


void udc_memset_00(void * dst, size_t len)
{
    memset(dst, 0, len);
}


