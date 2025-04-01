#ifndef __UDC_H__
#define __UDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "./hal/udc_hal.h"
#include "./core/udc_core.h"
#include "./core/udc_event.h"


/**
 *  包头 - 包长 - 数据 - 校验
 *               数据 ：数据id - 数据类型 - 数据
 *         包长：2字节 低位在前, 高位在后 (包长后面的数据)
 */

#ifdef __cplusplus
}
#endif

#endif /* __UDC_H__ */
