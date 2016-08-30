/*==============================================================================
** calculator.h -- calculator interface.
**
** MODIFY HISTORY:
**
** 2012-05-14 wdf Create.
==============================================================================*/

#ifndef __CALCULATOR_H__
#define __CALCULATOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//#define CAL_TYPE_INT

#ifdef CAL_TYPE_INT
#define CAL_TYPE  int
#else /* CAL_TYPE_INT */
#define CAL_TYPE  double
#endif /* CAL_TYPE_INT */

CAL_TYPE calculate (char *expression);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CALCULATOR_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

