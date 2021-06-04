/*==============================================================================
** config.h -- system config.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "bsp/bsp_config.h"

/*======================================================================
  kernel version
======================================================================*/
#define DFEW_KERNEL_VERSION     "DFEW version 0.1.0"

/*======================================================================
  utils
======================================================================*/
#define BIT_SET(val, pos)       ((val) |= (1 << (pos)))
#define BIT_CLEAR(val, pos)     ((val) &= ~(1 << (pos)))
#define BIT_VALUE(val, pos)     (((val) >> (pos)) & 0x1)

#define MIN(a, b)               (((a) > (b)) ? (b) : (a))
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))

#define N_ELEMENTS(array)       (sizeof(array) / sizeof((array)[0]))

#define FOREVER                 for (;;)

/*======================================================================
  Software CONFIG
======================================================================*/
#define PATH_LEN_MAX            256  /* path length                   */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CONFIG_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

