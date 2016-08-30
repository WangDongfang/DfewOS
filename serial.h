/*==============================================================================
** serial.h -- serial module header.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef  __SERIAL_H__
#define  __SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  serial APIs
======================================================================*/
int serial_init(void);
int serial_getc(void);
int serial_putc(const char c);
int serial_tstc(void);
int serial_puts(const char *s);
int serial_read(char *buf, int max);
int serial_write(char *buf, int e_size, int e_num);
int serial_printf(const char* str, ... );
int serial_pend(int ticks);

/**********************************************************************************************************
  VT100+ Color codes
**********************************************************************************************************/
#define COLOR_FG_BLACK  "\033[30m"                                      /*  ºÚ   ×Ö                      */
#define COLOR_FG_RED    "\033[31m"                                      /*  ºì   ×Ö                      */
#define COLOR_FG_GREEN  "\033[32m"                                      /*  ÂÌ   ×Ö                      */
#define COLOR_FG_YELLOW "\033[33m"                                      /*  »Æ   ×Ö                      */
#define COLOR_FG_BLUE   "\033[34m"                                      /*  À¶   ×Ö                      */
#define COLOR_FG_MAGEN  "\033[35m"                                      /*  Æ·ºì ×Ö                      */
#define COLOR_FG_CYAN   "\033[36m"                                      /*  Çà   ×Ö                      */
#define COLOR_FG_WHITE  "\033[37m"                                      /*  °×   ×Ö                      */


#define COLOR_BG_BLACK  "\033[40m"                                      /*  ºÚ   µ×                      */
#define COLOR_BG_RED    "\033[41m"                                      /*  ºì   µ×                      */
#define COLOR_BG_GREEN  "\033[42m"                                      /*  ÂÌ   µ×                      */
#define COLOR_BG_YELLOW "\033[43m"                                      /*  »Æ   µ×                      */
#define COLOR_BG_BLUE   "\033[44m"                                      /*  À¶   µ×                      */
#define COLOR_BG_MAGEN  "\033[45m"                                      /*  Æ·ºì µ×                      */
#define COLOR_BG_CYAN   "\033[46m"                                      /*  Çà   µ×                      */
#define COLOR_BG_WHITE  "\033[47m"                                      /*  °×   µ×                      */

#define CLEAR_SCREEN    "\033[H\033[0J"
#define HIDE_CURSOR     "\033[?25l"
#define SHOW_CURSOR     "\033[?25h"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SERIAL_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

