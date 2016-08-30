/*==============================================================================
** shell.c -- a simple shell.
**
** MODIFY HISTORY:
**
** 2011-08-13 wdf Create.
** 2011-10-09 wdf Add history record.
==============================================================================*/
#include "serial.h"
#include "types.h"
#include "config.h"
#include "dlist.h"
#include "task.h"
#include "readyQ.h"
#include "delayQ.h"
#include "msgQ.h"
#include "semB.h"
#include "semM.h"
#include "string.h"
#include "cmd.h"
#include "fifo.h"

#include <malloc.h>
int sscanf(const char *str, char const *fmt, ...);

/*======================================================================
  configs
======================================================================*/
#define _CMD_LEN_MAX    1024
#define _HISTORY_MAX    3

/*======================================================================
  Command line history node struct
======================================================================*/
typedef struct _shell_history_node {
    DL_NODE history_node;

    char    cmd_line[_CMD_LEN_MAX];
} _SHELL_HISTORY_NODE;

/*======================================================================
  other module command init routines declare
======================================================================*/
extern void vi_cmd_init();
extern void tetris_cmd_init();
extern void lian_cmd_init();
extern void date_cmd_init();
extern void usb_cmd_init();
extern void fat_cmd_init();
extern int  yaffs_init();
extern void net_init();
extern int  gui_init();
extern void cal_cmd_init();
extern void ios_init();
extern void media_init();
void wtd_cmd_init();

/*======================================================================
  forward function declare
======================================================================*/
static char *_del_front_space(char *cmd_line);
static void  _shell_cmd_init();
static int   _get_cmd_line(char *s);

/*==============================================================================
 * - kernel_version()
 *
 * - return current kernel version by string
 */
char *kernel_version ()
{
    return DFEW_KERNEL_VERSION;
}

/*==============================================================================
 * - banner()
 *
 * - print logo
 */
void banner (void)
{
    char *logo[] =
    {
"\n",
"                                 ]]]]",
"                               ]]]]",
"                              ]]]",
"                             ]]]",
"                            ]]]",
"                           ]]]",
"                          ]]]",
"]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]",
"                        ]]]",
"            ]]]]]]     ]]]             ]]]",
"          ]]]     ]]] ]]]]              ]]]",
"        ]]]          ]]]]]      ]]]      ]]",
"        ]]]         ]]] ]]     ]] ]]     ]]",
"        ]]]        ]]]  ]]]   ]]  ]]    ]]",
"          ]]]    ]]]     ]]] ]]    ]]  ]]",
"            ]]]]]]        ]]]]      ]]]]         (TM)",
"\n"
    };
    int ix;

    for (ix = 0; ix < (int)N_ELEMENTS(logo); ix++)
        serial_printf (" %s\n", logo [ix]);

    serial_printf (" %s%s %s\n",
            "]]]]]]]]]]]]]]]]]]]]]] ", "DfewOS", "1.0.0");
    serial_printf (" %s%s\n",
            "]]]]]]]]]]]]]]]]]]]    KERNEL: ", kernel_version ());
    serial_printf (" %s\n",
            "]]]]]]]]]]]]]]]]       Copyright D few, Inc., 1988-2011");

    serial_printf ("\n");
}

/*==============================================================================
 * - T_shell()
 *
 * - the shell task entry
 */
void T_shell ()
{
    char usr_cmd_line[_CMD_LEN_MAX];
    char *start_cmd = NULL;

    banner();

    _shell_cmd_init();      /* i, exit ... */

    yaffs_init();           /* cd, ls, pwd ... */
    vi_cmd_init();          /* vi */
    net_init();
    tetris_cmd_init();      /* tetris */
    lian_cmd_init();        /* lianliankan */
    date_cmd_init();        /* show and set date */
    cal_cmd_init();			/* calculator expression */
    gui_init();
    ios_init();
    media_init();
    wtd_cmd_init();
#if 0
    fat_cmd_init();         /* fatls */
    usb_cmd_init();         /* usb.start ... */
#endif

    serial_puts("\n-> ");


    FOREVER {
        _get_cmd_line(usr_cmd_line);

        start_cmd = _del_front_space(usr_cmd_line);

        /*
         * cmd_line have no char except blank
         */
        if (strlen(start_cmd) == 0) {
            serial_puts("-> ");
            continue;
        }

        /*
         * deal with cmd_line
         */
        if (cmd_do(start_cmd) == CMD_EXIT) {
            serial_puts("\nAu Revoir!\n");
            break;
        } else {
            serial_puts("\n-> ");
        }
    }
}
/*
int isspace (int c)
{
	return  ((c == ' ') || (c == '\t') || (c == '\n'));
}*/
/*==============================================================================
 * - _del_front_space()
 *
 * - find the first no space char position
 */
static char *_del_front_space(char *cmd_line)
{
    while (isspace(*cmd_line)) {
        cmd_line++;
    }

    return cmd_line;
}

static int _get_last_cmd_len (const char *cmd_line)
{
    char *pblank;
    int   len = strlen(cmd_line);

    pblank = memrchr (cmd_line, ' ', len);
    if (pblank == NULL) {
        return len;
    } else {
        return len - (pblank - cmd_line + 1);
    }
}

/*==============================================================================
 * - _load_history()
 *
 * - load a history command line to current command line
 */
static int _load_history (char *cmd_line, int old_len, _SHELL_HISTORY_NODE *p_history_node)
{
    if (p_history_node != NULL) {
        /* erase */
        while (old_len--) serial_puts("\b \b");
        /* load */
        strcpy (cmd_line, p_history_node->cmd_line);
        serial_puts(cmd_line);
        old_len = strlen (cmd_line);
    }

    return old_len;
}
/*==============================================================================
 * - _get_cmd_line()
 *
 * - get user typed string from uart0
 */
int _get_cmd_line (char *s)
{
    int i = 0;  /* readed char number so far */
    int c; /* temp read char */
    char tab_matched_cmd[PATH_LEN_MAX] = "";
    int have_tab = 0;

    _SHELL_HISTORY_NODE *p_history_index = NULL;
    _SHELL_HISTORY_NODE *p_history_node  = NULL;

    static DL_LIST history_list = {NULL, NULL};
    static int     history_num = 0;

    while (i < _CMD_LEN_MAX) {
        c = serial_getc();

        /* check Tab key */
        if ((c != '\t') && have_tab) {
        	int s_len = strlen (s);
        	int prefix_len;
        	prefix_len = _get_last_cmd_len (s);
            strcpy (&s[s_len - prefix_len], tab_matched_cmd);
            i = strlen (s);
            have_tab = 0;
        }

        switch (c) {
            case '\r':                      /* Enter */
                s[i] = '\0';
                serial_putc('\n');
                goto got_it;
            case '\b':                      /* Backspace */
            	if (i >= 1) {
                    serial_puts("\b \b");
                    i--;
            	}
                break;
            case '\t':                      /* Tab */
                have_tab = 1;
                s[i] = '\0';
#if 0
                /* erase */
                while (i--) serial_puts("\b \b");
                i = cmd_tab(s, tab_matched_cmd);
                serial_puts(tab_matched_cmd);
#else
            {
                extern int yaffs_tab (const char *prefix, char *cmd_line);
                int prefix_len;
                int erase_chars;
                prefix_len = _get_last_cmd_len (s);
                erase_chars = i - (strlen (s) - prefix_len);
                if (erase_chars == i) {
                    i = cmd_tab(s, tab_matched_cmd);
                } else {
                    i -= erase_chars;
                    i += yaffs_tab(&s[i], tab_matched_cmd);
                }
                while (erase_chars--) serial_puts("\b \b");
                serial_puts(tab_matched_cmd);
            }
#endif
                break; /* ignore */
            case '\033':                    /* Esc Up Down Right Left */
                if (serial_tstc() && (serial_getc() == '[')) {
                    if (serial_tstc()) {

                        c = serial_getc();
                        switch (c) {
                            case 'A':    /* Up */
                                if (p_history_index == NULL) {
                                    p_history_index = (_SHELL_HISTORY_NODE *)DL_LAST(&history_list);
                                } else {
                                    p_history_index = (_SHELL_HISTORY_NODE *)DL_PREVIOUS(p_history_index);
                                }
                                i = _load_history (s, i, p_history_index);
                                break;
                            case 'B':    /* Down */
                                if (p_history_index != NULL) {
                                    p_history_index = (_SHELL_HISTORY_NODE *)DL_NEXT(p_history_index);
                                }
                                i = _load_history (s, i, p_history_index);
                                break;
                            case 'C':    /* Right */
                                break;
                            case 'D':    /* Left */
                                break;
                            default:
                                break;
                        }
                    }
                }
                break;
            default:                        /* other */
                s[i++] = c;
                serial_putc(c);
                break;
        }
    }

    /* user command line length more than 1023 */
    if (i == _CMD_LEN_MAX) {
        s[_CMD_LEN_MAX - 1] = '\0';
        i--;
    }

got_it:
    /* this command is not NULL and not equal last command */
    if ((i != 0) && 
        ((DL_EMPTY(&history_list)) || (strcmp (s, ((_SHELL_HISTORY_NODE *)DL_LAST(&history_list))->cmd_line)))
       ) {

        if (history_num < _HISTORY_MAX) {
            p_history_node = malloc (sizeof (_SHELL_HISTORY_NODE));
            history_num++;
        } else {
            p_history_node = (_SHELL_HISTORY_NODE*)dlist_get(&history_list);
        }
        strcpy (p_history_node->cmd_line, s);
        dlist_add (&history_list, (DL_NODE *)p_history_node);
    }

    return i;
}

/**********************************************************************************************************
  command operater
 **********************************************************************************************************/
static int _shell_exit ()
{
    return CMD_EXIT;
}

/*==============================================================================
 * - C_show_memory()
 *
 * - show memory value
 *   usage: -> d 0x30010000 [8]
 */
int C_show_memory (int argc, char **argv)
{
    uint32           times = 1;
    volatile uint32 *addr = NULL;

    /* read memory start Address */
    if (argc >= 2) {
        sscanf (argv[1], "0x%X", (unsigned int *)&addr);
    }
        
    /* read show times */
    if (argc >= 3) {
        sscanf (argv[2], "%d", &times);
    }

    /* show */
    while (times--) {
        serial_printf ("*(0x%8X) = 0x%8X\n", addr, *addr);
        addr++;
    }

    return CMD_OK;
}

/*==============================================================================
 * - C_set_memory()
 *
 * - set memory value
 *   usage: -> s 0x30010000 0x2 [8]
 */
int C_set_memory (int argc, char **argv)
{
    volatile uint32 *addr = NULL;
    uint32           value = 0;
    uint32           times = 1;

    /* read memory start Address */
    if (argc >= 3) {
        sscanf (argv[1], "0x%X", (unsigned int *)&addr);
        sscanf (argv[2], "0x%X", &value);
        *addr = value;
    } else {
        return CMD_ERROR;
    }

    /* read set times */
    if (argc >= 4) {
        sscanf (argv[3], "%d", &times);
    }

    /* set */
    while (times--) {
        *addr = value;
        addr++;
    }

    return CMD_OK;
}

/*======================================================================
  command "pp" "gg". Example FIFO
======================================================================*/
#define _FIFO_NUM        3
static  FIFO *_G_p_fifo = NULL;
int C_put_fifo (int argc, char **argv)
{
    int value = 0;
    int i = 1; /* value start index in argv */

    if (_G_p_fifo == NULL) {
        _G_p_fifo = fifo_create (_FIFO_NUM, sizeof (int));
    }

    /* create FIFO failed */
    if (_G_p_fifo == NULL) {
        serial_printf("Create FIFO failed!");
        return CMD_ERROR;
    }

    /* read the put values */
    for (; i < argc; i++) {
        value = 0;
        sscanf (argv[i], "%d", (unsigned int *)&value);
        if (fifo_put (_G_p_fifo, &value) == OS_STATUS_ERROR) {
            serial_printf("The FIFO is full!");
            return CMD_ERROR;
        }
        serial_printf("Put: %d\n", value);
    }

    serial_printf("The put operation is Done.");
    return CMD_OK;
}
int C_get_fifo (int argc, char **argv)
{
    int value = -1;
    int times = 1;

    if (_G_p_fifo == NULL) {
        serial_printf("The FIFO is not create!");
        return CMD_ERROR;
    }

    /* read times */
    if (argc >= 2) {
        sscanf (argv[1], "%d", (unsigned int *)&times);
    }

    while (times--) {
        if (fifo_get (_G_p_fifo, &value) == OS_STATUS_ERROR) {
            serial_printf("The FIFO is empty!");
            return CMD_ERROR;
        }
        serial_printf("Get: %d\n", value);
    }

    serial_printf("The get operation is Done.");
    return CMD_OK;
}

/*==============================================================================
 * - C_reboot()
 *
 * - reboot
 */
int C_reboot (int argc, char **argv)
{
    __asm__ (
        "ldr	r1, =0x7e00f000\n"
        "ldr	r2, [r1, #0x118]\n"
        "ldr	r3, =0xffff\n"
        "and	r2, r3, r2, lsr #12\n"
        "str	r2, [r1, #0x114]\n"
        "_loop_forever:\n"
        "b	_loop_forever\n");

    return CMD_OK;
}

/**********************************************************************************************************
  init the origin commands, such as "i" "exit" "banner"
**********************************************************************************************************/
int cc(); int dd(); int b(); int c(); int m(); int q();
int h();
int C_show_memory (int argc, char **argv);
int C_set_memory (int argc, char **argv);
static CMD_INF _G_shell_origin_cmds[] = {
    {"help",   "show command information", (CMD_FUNC)cmd_show},
    {"i",      "show task information", (CMD_FUNC)task_show},
    {"b",      "show semB information", (CMD_FUNC)b},
    {"c",      "show semC information", (CMD_FUNC)c},
    {"m",      "show semM information", (CMD_FUNC)m},
    {"q",      "show msgQ information", (CMD_FUNC)q},
    {"h",      "show heap information", (CMD_FUNC)h},
    {"d",      "show memory value",     (CMD_FUNC)C_show_memory},
    {"s",      "set memory value",      (CMD_FUNC)C_set_memory},
    {"banner", "show the os banner",    (CMD_FUNC)banner},
    {"cc",     "create a delay task",   (CMD_FUNC)cc},
    {"dd",     "over all delay task",   (CMD_FUNC)dd},
    {"pp",     "put integer into FIFO", (CMD_FUNC)C_put_fifo},
    {"gg",     "get integer from FIFO", (CMD_FUNC)C_get_fifo},
    {"exit",   "exit this shell",       (CMD_FUNC)_shell_exit},
    {"reboot", "reboot DfewOS",         (CMD_FUNC)C_reboot}
};

static void _shell_cmd_init()
{
    int i;
    int cmd_num;
    cmd_num = N_ELEMENTS(_G_shell_origin_cmds);

    for (i = 0; i < cmd_num; i++) {
        cmd_add(&_G_shell_origin_cmds[i]);
    }
}

/*==============================================================================
** FILE END
==============================================================================*/
