#ifndef __UDC_H__
#define __UDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "./src/hal/udc_hal.h"
#include "./src/core/udc_core.h"
#include "./src/core/udc_event.h"
#include "./src/misc/udc_mem.h"


/**
 *  包头 - 包长 - 数据 - 校验
 * 
 * 包长     : 2字节 高位在前，低位在后,  (包长后面的数据)
 * 数据     : 数据id - 数据长度 - 数据
 * 数据id   : 1字节；如id=01，当数据长度<=255时，数据id=1，否则传输id = 1+120
 * 数据长度 : 高位在前，低位在后；当数据长度<=255时，数据长度为1字节，否则数据长度为2字节
 */

#ifdef __cplusplus
}
#endif

#endif /* __UDC_H__ */
