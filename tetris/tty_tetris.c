/*==============================================================================
** tty_tetris.c -- a tetris game in tty.
**
** MODIFY HISTORY:
**
** 2011-09-29 wdf Create.
==============================================================================*/

#include <dfewos.h>

/*======================================================================
  configs
======================================================================*/
#define NODE_ELEMENT    "__" /* block's node. it consist of two char */
#define COL_START       2
#define ROW_START       4
#define COL_NUM         16   /* left -- right width */   
#define ROW_NUM         26   /* top -- bottom width */

#define BLOCK_HEIGHT    1 
#define BLOCK_WIDTH     2

/*======================================================================
  Bridge module API declare
======================================================================*/
void Russian_main (void *Display_ID,
      int fd,
      int left_top_x, int left_top_y,
      int right_bottom_x, int right_bottom_y,
      int node_width,
      int (*dsply_show_routine)(void *Display_ID, int x0, int y0, int x1, int y1, int show));


//----- show or erase a block on tty --------------------------------
static int _Russian_tty_show (void *Display_ID, int x0, int y0, int x1, int y1, int show)
{
    if (show & 0x80000000) {  /* show score */
        int start_col;

        start_col = COL_START + ((COL_NUM * BLOCK_WIDTH) - 10) / 2;

        /* move cursor */
        serial_printf ("\033[%d;%dH", ROW_START - 2, start_col);

        /* set background & foreground color */
        serial_printf (COLOR_BG_BLACK);
        serial_printf (COLOR_FG_YELLOW);
        
        serial_printf ("Score: %d", show & 0x7FFFFFFF);

        return show;
    }

    /* move cursor */
    serial_printf ("\033[%d;%dH", y0 + ROW_START, x0*2 + COL_START);

    /* set foreground color as BLACK */
    serial_printf (COLOR_FG_BLACK);

    /* set background color */
    serial_printf ("\033[%dm", 40 + show);

    /* show or erase node */
    serial_printf (NODE_ELEMENT);

    return show;
}

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
void tetris_main (int argc, char **argv)
{
    /* clear screen and hide cursor */
    serial_printf (CLEAR_SCREEN);
    serial_printf (HIDE_CURSOR);

    /* draw frame */
    _draw_frame ();

    /* start tetris */
    Russian_main (NULL, 0, 0, 0, COL_NUM, ROW_NUM, 1, _Russian_tty_show);

    /* clear screen, show cursor and set cmd mode */
    serial_printf (COLOR_FG_WHITE);
    serial_printf (COLOR_BG_BLACK);
    serial_printf (CLEAR_SCREEN);
    serial_printf (SHOW_CURSOR);
}

/*==============================================================================
** FILE END
==============================================================================*/

