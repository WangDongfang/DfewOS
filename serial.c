/*==============================================================================
** serial.c -- DfewOS serial communicate routines.
**
** MODIFY HISTORY:
**
** 2012-03-02 wdf Create.
==============================================================================*/
#include "types.h"
#include "config.h"
#include "dlist.h"
#include "task.h"
#include "semM.h"
#include "bsp/uart.h"

/*======================================================================
  FIFO lock
======================================================================*/
#if 0
static  SEM_MUX                 _G_serial_semM;
#define SERIAL_FIFO_LOCK_INIT() semM_init(&_G_serial_semM)
#define SERIAL_FIFO_LOCK()      semM_take(&_G_serial_semM, WAIT_FOREVER)
#define SERIAL_FIFO_UNLOCK()    semM_give(&_G_serial_semM)
#else
#define SERIAL_FIFO_LOCK_INIT()
#define SERIAL_FIFO_LOCK()
#define SERIAL_FIFO_UNLOCK() 
#endif

/*==============================================================================
 * - serial_init()
 *
 * - init UART hardware
 */
int serial_init()
{
    uart_init ();
    SERIAL_FIFO_LOCK_INIT();

    return 0;
}

/*==============================================================================
 * - serial_getc()
 *
 * - get a char
 */
int serial_getc(void)
{
    return uart_getc();
}

/*==============================================================================
 * - serial_putc()
 *
 * - send a char
 */
int serial_putc(const char c)
{
    return uart_putc(c);
}

/*==============================================================================
 * - serial_tstc()
 *
 * - check whether have received char in FIFO
 */
int serial_tstc(void)
{
    return uart_tstc ();
}

/*==============================================================================
 * - serial_puts()
 *
 * - send a string
 */
int serial_puts(const char *s)
{
    int i = 0;

    SERIAL_FIFO_LOCK();
    while (*s) {
        serial_putc(*s++);
        i++;
    }
    SERIAL_FIFO_UNLOCK();
    return i;
}

/*==============================================================================
 * - serial_pend()
 *
 * - wait some ticks for receive data
 */
int serial_pend (int ticks)
{
    return uart_pend (ticks);
}

/*==============================================================================
 * - serial_read()
 *
 * - try to receive <max> char from FIFO
 */
int serial_read (char *buf, int max)
{
    int nbyte = 0;

    if (serial_tstc() == 0)
        uart_pend (WAIT_FOREVER);

    SERIAL_FIFO_LOCK();
    while ((uart_tstc() != 0) && (max-- != 0)) {
        *buf++ = uart_getc();
        nbyte++;
    }
    SERIAL_FIFO_UNLOCK();

    return nbyte;
}

/*==============================================================================
 * - serial_write()
 *
 * - send a buffer
 */
int serial_write (char *buf, int e_size, int e_num)
{
    int i;
    int max = e_size * e_num;

    SERIAL_FIFO_LOCK();
    for (i = 0; i < max; i++) {
        serial_putc(buf[i]);
    }
    SERIAL_FIFO_UNLOCK();

    return i;
}

/**********************************************************************************************************
  printf
**********************************************************************************************************/
#define f_putc(c, f)  serial_putc(c)
#define f_puts(s, f)  serial_puts(s)
#define ULONG           unsigned long
#define UCHAR           unsigned char
#ifndef EOF
#define EOF             -1
#endif

#include <stdarg.h>
int serial_printf (const char* str, ... )
{
    va_list arp;
    UCHAR c, f, r;
    ULONG val;
    char s[16];
    int i, w, res, cc;


    va_start(arp, str);

    SERIAL_FIFO_LOCK();
    for (cc = res = 0; cc != EOF; res += cc) {
        c = *str++;
        if (c == 0) break;            /* End of string */
        if (c != '%') {                /* Non escape cahracter */
            cc = f_putc(c, fil);
            if (cc != EOF) cc = 1;
            continue;
        }
        w = f = 0;
        c = *str++;
        if (c == '%') {             /* print '%' _Dong */
            cc = f_putc(c, fil);
            continue;
        }
        if (c == '0') {                /* Flag: '0' padding */
            f = 1; c = *str++;
        }
        if (c == ' ') {             /* Flag: ' ' padding _Dong */
            c = *str++;
        }
        while (c >= '0' && c <= '9') {    /* Precision */
            w = w * 10 + (c - '0');
            c = *str++;
        }
        if (c == 'l') {                /* Prefix: Size is long int */
            f |= 2; c = *str++;
        }
        if (c == 's') {                /* Type is string */
            cc = f_puts(va_arg(arp, char*), fil);
            continue;
        }
        if (c == 'c') {                /* Type is character */
            cc = f_putc(va_arg(arp, int), fil);
            if (cc != EOF) cc = 1;
            continue;
        }
        r = 0;
        if (c == 'd') r = 10;        /* Type is signed decimal */
        if (c == 'u') r = 10;        /* Type is unsigned decimal */
        if (c == 'X' || c == 'x' || c == 'p') r = 16;        /* Type is unsigned hexdecimal _Dong */
        if (r == 0) break;            /* Unknown type */
        if (f & 2) {                /* Get the value */
            val = (ULONG)va_arg(arp, long);
        } else {
            val = (c == 'd') ? (ULONG)(long)va_arg(arp, int) : (ULONG)va_arg(arp, unsigned int);
        }
        /* Put numeral string */
        if (c == 'd') {
            if (val & 0x80000000) {
                val = 0 - val;
                f |= 4;
            }
        }
        i = sizeof(s) - 1; s[i] = 0;
        do {
            c = (UCHAR)(val % r + '0');
            if (c > '9') c += 7;
            s[--i] = c;
            val /= r;
        } while (i && val);
        if (i && (f & 4)) s[--i] = '-';
        w = sizeof(s) - 1 - w;
        while (i && i > w) s[--i] = (f & 1) ? '0' : ' ';
        cc = f_puts(&s[i], fil);
    }

    SERIAL_FIFO_UNLOCK();
    va_end(arp);
    return (cc == EOF) ? cc : res;
}

/*==============================================================================
** FILE END
==============================================================================*/

