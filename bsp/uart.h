/*==============================================================================
** uart.h -- uart driver interface for DfewOS.
**
** MODIFY HISTORY:
**
** 2012-03-04 wdf Create.
==============================================================================*/

#ifndef __UART_H__
#define __UART_H__

#ifdef __cpulsplus
extern "C" {
#endif

int uart_init();
int uart_putc (const char c);
int uart_getc ();
int uart_tstc(void);
int uart_pend (int ticks);

#ifdef __cpulsplus
}
#endif

#endif /* __UART_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

