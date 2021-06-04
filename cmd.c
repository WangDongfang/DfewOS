/*==============================================================================
** cmd.c -- this module manage the command line
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/
#include "config.h"
#include "types.h"
#include "serial.h"
#include "dlist.h"
#include "task.h"
#include "string.h"
#include "cmd.h"

#include "memH.h"

/*======================================================================
  command node to linked in _G_cmd_list struct
======================================================================*/
typedef struct command_node {
    DL_NODE  cmd_list_node;

    CMD_INF *cmd_info;
} CMD_NODE;

/*======================================================================
  forward functions declare
======================================================================*/
static int _cmd_dispatch_arg(char *cmd_line);
static OS_STATUS _cmd_show_one (CMD_NODE *cmd_node, int unused);
static OS_STATUS _cmd_find_one (CMD_NODE *cmd_node, char *cmd_to_find);
static void _cmd_unknown_warning (char *cmd_name);
 
#define ARG_NUM_MAX   10
static char *_G_command;
static int   _G_argc;
static char *_G_argv[ARG_NUM_MAX];

/*======================================================================
  this list link all CMD_NODE sturct
======================================================================*/
static DL_LIST _G_cmd_list = {NULL, NULL};
static int _G_cpsr_c;
#define CMD_MANAGE_LOCK()    _G_cpsr_c = CPU_LOCK()
#define CMD_MANAGE_UNLOCK()  CPU_UNLOCK(_G_cpsr_c)

/*==============================================================================
 * - cmd_add()
 *
 * - add a new command to command list
 */
void cmd_add (CMD_INF *new_cmd_info)
{
    CMD_NODE *new_cmd_node = NULL;

    CMD_MANAGE_LOCK();
    new_cmd_node = (CMD_NODE *)malloc(sizeof(CMD_NODE));
    if (new_cmd_info != NULL) {
        new_cmd_node->cmd_info = new_cmd_info;
        dlist_add (&_G_cmd_list, (DL_NODE *)new_cmd_node);
    }
    CMD_MANAGE_UNLOCK();
}

/*==============================================================================
 * - cmd_do()
 *
 * - analyse cmd_line and call it's operate function
 *   <cmd_line> have chars & first char is not space
 */
int cmd_do (char *cmd_line)
{
    CMD_NODE *cmd_node = NULL;

    _cmd_dispatch_arg(cmd_line);  /* modify global variable "_G_command", "_G_argc", "_G_argv" */

    cmd_node = (CMD_NODE *)dlist_each(&_G_cmd_list, (EACH_FUNC_PTR)_cmd_find_one, (int)_G_command);
    if (cmd_node != NULL) {
        return (cmd_node->cmd_info->cmd_op_func)(_G_argc, _G_argv);  /* run command operat function */
    } else {
        _cmd_unknown_warning(_G_command);
        return CMD_BAD;
    }
}

/*==============================================================================
 * - cmd_show()
 *
 * - the "help" command operate function. print command(s) help
 */
int cmd_show (int argc, char *argv[])
{
    if (argc == 1) {
        dlist_each(&_G_cmd_list, (EACH_FUNC_PTR)_cmd_show_one, 0);
    } else {
        int i;
        CMD_NODE *cmd_node = NULL;
        for (i = 1; i < argc; i++) {
            cmd_node = (CMD_NODE *)dlist_each(&_G_cmd_list, (EACH_FUNC_PTR)_cmd_find_one, (int)argv[i]);
            if (cmd_node != NULL) {
                _cmd_show_one(cmd_node, 0);
            } else {
                _cmd_unknown_warning(argv[i]);
                serial_putc('\n');
            }
        }
    }
    
    return 0;
}

/*==============================================================================
 * - cmd_tab()
 *
 * - auto complete the command with <prefix> initial, store in <cmd_line>
 */
int cmd_tab (const char *prefix, char *cmd_line)
{
    static char      last_prefix[20] = "";
    static CMD_NODE *pLast = NULL;

    if (strcmp (last_prefix, prefix) != 0) { /* if user change prefix */
        strcpy (last_prefix, prefix);
        pLast = (CMD_NODE *)DL_FIRST(&_G_cmd_list);
    } else { /* use last prefix */
        if (pLast == NULL) {
            pLast = (CMD_NODE *)DL_FIRST(&_G_cmd_list);
        } else {
            pLast = (CMD_NODE *)DL_NEXT(pLast);
        }
    }

    while (pLast != NULL) {
        if (strncmp (prefix, pLast->cmd_info->cmd_name, strlen(prefix)) == 0) {
            strcpy (cmd_line, pLast->cmd_info->cmd_name);
            break;
        }
        pLast = (CMD_NODE *)DL_NEXT(pLast);
    }

    if (pLast == NULL) {
        strcpy (cmd_line, prefix);
    }

    return strlen (cmd_line);
}

/*==============================================================================
 * - _cmd_dispatch_arg ()
 *
 * - analyse the command line. 
 *   modify global variable "_G_command", "_G_argc", "_G_argv"
 */
static int _cmd_dispatch_arg (char *cmd_line)
{
    int after_blank = 1;

    _G_command = cmd_line;
    _G_argc = 0;

    while (*cmd_line != '\0') {
        if (isspace(*cmd_line)) {
            *cmd_line = '\0';
            after_blank = 1;
        } else {
            if (after_blank) {
                _G_argv[_G_argc] = cmd_line;
                _G_argc++;

                if (_G_argc == ARG_NUM_MAX) {
                    break;
                }
            }
            after_blank = 0;
        }
        cmd_line++;
    }

    return 0;
}

/*==============================================================================
 * - _cmd_unknown_warning()
 *
 * - a helper function. print unknown warning
 */
static void _cmd_unknown_warning (char *cmd_name)
{
    serial_puts("Sorry! I don't know : ");
    serial_puts(COLOR_FG_RED);
    serial_puts(cmd_name);
    serial_puts(COLOR_FG_WHITE);
}

/*==============================================================================
 * - _cmd_find_one()
 *
 * - check whether this node is the find one, used by dlist_each
 *
 * - RETURN: if this node is the find one return OS_STATUS_ERROR, make the 
 *           dlist_each return this node's pointer, otherwise return 
 *           OS_STATUS_OK, make the dlist_each continue find next node
 */
static OS_STATUS _cmd_find_one (CMD_NODE *cmd_node, char *cmd_to_find)
{
    if (strcmp((char *)cmd_to_find, cmd_node->cmd_info->cmd_name) == 0) {
        /* find it, stop dlist_each */
        return OS_STATUS_ERROR;
    }
    return OS_STATUS_OK;
}

/*==============================================================================
 * - _cmd_show_one()
 *
 * - print a cmd node information
 */
static OS_STATUS _cmd_show_one (CMD_NODE *cmd_node, int unused)
{
    serial_printf("%s\t : %s\n",
                  cmd_node->cmd_info->cmd_name,
                  cmd_node->cmd_info->cmd_desc);

    return OS_STATUS_OK;
}

/*==============================================================================
** FILE END
==============================================================================*/

