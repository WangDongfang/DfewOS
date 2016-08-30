/*==============================================================================
** tty_lian.c -- a lianliankan game in tty.
**
** MODIFY HISTORY:
**
** 2011-10-13 wdf Create.
==============================================================================*/

#include "tty.h"

#ifdef OS_DFEWOS /* OS_DFEWOS */
#include <dfewos.h>
#include "../../string.h"
#define rawmode()
#define cookmode()
#else            /* OS_VXWORKS */
#include <vxWorks.h>
#include <ioLib.h>
#include <stdio.h>
#include <string.h>
#define  serial_printf          printf
#define  serial_putc(c)         printf("%c", c)
#define  serial_puts(s)         printf(s);
#define  serial_read(key, n)    read(0, key, n);

#define  CLEAR_SCREEN    "\033[H\033[0J"
#define  HIDE_CURSOR     "\033[?25l"
#define  SHOW_CURSOR     "\033[?25h"
#define  COLOR_FG_BLACK  "\033[30m"        /*  ºÚ   ×Ö                      */
#define  COLOR_FG_WHITE  "\033[37m"        /*  °×   ×Ö                      */
#define  COLOR_FG_YELLOW "\033[33m"        /*  »Æ   ×Ö                      */
#define  COLOR_BG_BLACK  "\033[40m"        /*  ºÚ   µ×                      */
#endif          /* OS_DFEWOS */

/*======================================================================
  configs
======================================================================*/
static char *_G_fill[] = {"  ", "AA", "BB", "CC", "DD", "EE", "FF"};

#define MOVE_SIGN       "&"
#define MARK_SIGN       "@"

#define ROW_START        4
#define COL_START        2

#define BLOCK_HEIGHT     2 
#define BLOCK_WIDTH      4

/*======================================================================
  Utils
======================================================================*/
#define LIAN_SWAP(a, b)   {int t = a; a = b; b = t;}

/*======================================================================
  KeyBoard Strings
======================================================================*/
#define KEY_ESC     "\033"
#define KEY_UP      "\033[A"
#define KEY_DOWN    "\033[B"
#define KEY_RIGHT   "\033[C"
#define KEY_LEFT    "\033[D"
#define KEY_ENTER   "\r"
#define KEY_TAB     "\t"
#define KEY_B       "B"
#define KEY_WAVE    "`"         /* the key below [ESC] and left [1] */

/*======================================================================
  Beep control
======================================================================*/
static int _G_beep_on = 0;

/*======================================================================
  Bridge module API declare
======================================================================*/
int bridge_init ();
int bridge_next ();
int bridge_help ();
int bridge_move (int dir);
int bridge_mark ();
void bridge_exit ();

#ifndef OS_DFEWOS
//----- Set terminal attributes --------------------------------
static void rawmode(void)
{
    ioctl(0, FIOSETOPTIONS, OPT_RAW);
    ioctl(1, FIOSETOPTIONS, OPT_RAW);
}

static void cookmode(void)
{
	fflush(stdout);

	ioctl(0, FIOSETOPTIONS, OPT_TERMINAL);
    ioctl(1, FIOSETOPTIONS, OPT_TERMINAL);
}
#endif /* OS_DFEWOS */

//----- show or erase a block on tty --------------------------------
int tty_show (int row, int col, int type, int action)
{
    int start_row;
    int start_col;
    char *fill_str = NULL;

    start_row = (row - 1) * BLOCK_HEIGHT + ROW_START;
    start_col = (col - 1) * BLOCK_WIDTH + COL_START;

    if ((type - 1) % 7 == 3) { /* background color is BLUE */
        /* set foreground color as YELLOW */
        serial_printf (COLOR_FG_YELLOW);
    } else {
        /* set foreground color as BLACK */
        serial_printf (COLOR_FG_BLACK);
    }

    /* move cursor */
    serial_printf ("\033[%d;%dH", start_row, start_col);


    /* set background by <type> */
    serial_printf ("\033[%dm", 41 + (type - 1) % 7);

    switch (action) {
        case ACT_MARK:
            serial_printf (MARK_SIGN);
            break;
        case ACT_MOVE:
            serial_printf (MOVE_SIGN);
            break;
        case ACT_SHOW:
            if (type == 0) {
                serial_printf (COLOR_BG_BLACK);
                fill_str = _G_fill[type];
            } else {
                /* find fill string */
                fill_str = _G_fill[(type - 1) / 7];
            }
            serial_printf (fill_str);
            serial_printf (fill_str);
            serial_printf ("\033[%d;%dH", start_row + 1, start_col);
            serial_printf (fill_str);
            serial_printf (fill_str);
            break;
        default:
            break;
    }

    return type;
}

//----- draw line between two blocks ------------------------
int tty_line (int row0, int col0, int row1, int col1, int type)
{
    int i;
    int start_row;
    int start_col;
    int cnt;

    /* set foreground color and background color */
    serial_printf ("\033[%dm", 31 + (type - 1) % 7);
    serial_printf (COLOR_BG_BLACK);

    if (row0 > row1) {
        LIAN_SWAP(row0, row1);
    }

    if (col0 > col1) {
        LIAN_SWAP(col0, col1);
    }

    /* draw horizontal line */
    if (row0 == row1) {
        start_row = (row0 - 1) * BLOCK_HEIGHT + ROW_START;
        start_col = (col0 - 1) * BLOCK_WIDTH + COL_START + 1;
        cnt = (col1 - col0 - 1) * BLOCK_WIDTH + 3;

        /* move cursor to start */
        serial_printf ("\033[%d;%dH", start_row, start_col);
        serial_putc ('+');
        for (i = 0; i < cnt; i++) {
            serial_printf ("-");
        }
        serial_putc ('+');
    }

    /* draw vertial line */
    if (col0 == col1) {
        start_row = (row0 - 1) * BLOCK_HEIGHT + ROW_START + 1;
        start_col = (col0 - 1) * BLOCK_WIDTH + COL_START + 1;
        cnt = (row1 - row0 - 1) * BLOCK_HEIGHT + 1;

        /* move cursor to start */
        serial_printf ("\033[%d;%dH", start_row, start_col);
        for (i = 0; i < cnt; i++) {
            serial_printf ("|\b\033[B");
        }
    }

    return 0;
}

//----- show message ------------------------
int tty_msg (char *msg)
{
    int start_col = COL_START + ((COL_NUM * BLOCK_WIDTH) - strlen (msg)) / 2;

    /* move cursor */
    serial_printf ("\033[%d;%dH", ROW_START - 2, start_col);

    /* set color */
    serial_printf (COLOR_BG_BLACK);
    serial_printf (COLOR_FG_YELLOW);

    /* draw message string */
    serial_printf (msg);

    return 0;
}

//----- beep alarm ------------------------
#ifdef OS_DFEWOS  /* OS_DFEWOS */
extern void beep ();
int tty_beep (int times)
{
    if (_G_beep_on) {
        while (times--) beep();
    }
    return 0;
}
#else             /* OS_VXWORKS */
int tty_beep (int times) {return 0;}
#endif /* OS_DFEWOS */

//----- draw frame --------------------------------
/* The Frame looks like this: */
/*
   +------------------+
   |                  |
   +------------------+
   |                  |
   |                  |
   |                  |
   |                  |
   |                  |
   |                  |
   |                  |
   |                  |
   |                  |
   +------------------+
*/
static void _draw_frame ()
{
    int i ;

    /*
     * draw the hat, this area is for show message
     */
    
    /* move cursor to left top - 2 */
    serial_printf ("\033[%d;%dH", ROW_START - 3, COL_START - 1);
    /* +--------------+ */
    serial_putc ('+');
    for (i = 0; i < COL_NUM * BLOCK_WIDTH; i++) serial_putc ('-');
    serial_putc ('+');

    serial_printf ("\033[%d;%dH", ROW_START - 2, COL_START - 1);
    serial_putc ('|');
    serial_printf ("\033[%d;%dH", ROW_START - 2, COL_START + COL_NUM * BLOCK_WIDTH);
    serial_putc ('|');

    /*
     * draw the body, this area is for game
     */

    /* move cursor to left top */
    serial_printf ("\033[%d;%dH", ROW_START - 1, COL_START - 1);
    /* +--------------+ */
    serial_putc ('+');
    for (i = 0; i < COL_NUM * BLOCK_WIDTH; i++) serial_putc ('-');
    serial_putc ('+');

    /* move cursor to bottom top */
    serial_printf ("\033[%d;%dH", ROW_START + ROW_NUM * BLOCK_HEIGHT, COL_START - 1);
    /* +--------------+ */
    serial_putc ('+');
    for (i = 0; i < COL_NUM * BLOCK_WIDTH; i++) serial_putc ('-');
    serial_putc ('+');

    /* draw left line */
    serial_printf ("\033[%d;%dH", ROW_START, COL_START - 1);
    for (i = 0; i < ROW_NUM * BLOCK_HEIGHT; i++) {
        serial_putc ('|');
        serial_puts ("\033[B\b"); /* move cursor down */
    }

    /* draw right line */
    serial_printf ("\033[%d;%dH", ROW_START, COL_START + COL_NUM * BLOCK_WIDTH);
    for (i = 0; i < ROW_NUM * BLOCK_HEIGHT; i++) {
        serial_putc ('|');
        serial_puts ("\033[B\b"); /* move cursor down */
    }
}

//----- Main --------------------------------
void lian_main (int argc, char **argv)
{
	int nread;
    char key[4];

    rawmode();
    /* clear screen and hide cursor */
    serial_printf (CLEAR_SCREEN);
    serial_printf (HIDE_CURSOR);

    /* draw frame */
    _draw_frame ();

    /* start lianliankan */
    if (bridge_init() < 0) {
        return;
    }

    FOREVER {
        nread = serial_read (key, 3);
        key[nread] = '\0';

        if (strcmp (key, KEY_LEFT) == 0) {          /* Left */
            bridge_move (LEFT);
        } else if (strcmp (key, KEY_UP) == 0) {     /* Up */
            bridge_move (UP);
        } else if (strcmp (key, KEY_RIGHT) == 0) {  /* Right */
            bridge_move (RIGHT);
        } else if (strcmp (key, KEY_DOWN) == 0) {   /* Down */
            bridge_move (DOWN);
        } else if (strcmp (key, KEY_ENTER) == 0) {  /* Enter */
            bridge_mark ();
        } else if (strcmp (key, KEY_TAB) == 0) {    /* Tab */
            bridge_next ();
        } else if ((nread == 1) && (key[0] >= '1' && key[0] <= '9')) { /* 1 -- 9 */
            int help_times = key[0] - '0';
            nread = serial_read (key, 3);
            key[nread] = '\0';
            if (strcmp (key, KEY_WAVE) == 0) {  /* ` */
                while (help_times--) bridge_help ();
            }
        } else if (strcmp (key, KEY_WAVE) == 0) {   /* ` */
            bridge_help ();
        } else if (strcmp (key, KEY_ESC) == 0) {    /* Esc */
            bridge_exit ();
            break;
        } else if (strcmp (key, KEY_B) == 0) {      /* B */
            _G_beep_on = 1 - _G_beep_on;
        }
    }

    
    /* clear screen, show cursor and set cmd mode */
    serial_printf (COLOR_FG_WHITE);
    serial_printf (COLOR_BG_BLACK);
    serial_printf (CLEAR_SCREEN);
    serial_printf (SHOW_CURSOR);
    cookmode();
}

/*==============================================================================
** FILE END
==============================================================================*/

