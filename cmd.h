/*==============================================================================
** cmd.h -- the shell command line module header
**
** MODIFY HISTORY:
**
** 2011-8-13 wdf Create.
==============================================================================*/

#ifndef __CMD_H__
#define __CMD_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  the command operate function return value
======================================================================*/
#define CMD_EXIT  -1        /* the "exit" command return value */
#define CMD_BAD	  -2        /* the cmd not find */
#define CMD_ERROR -3        /* the cmd operater failed */
#define CMD_OK     0        /* the cmd operater successed */

/*======================================================================
  the command operate function type define
======================================================================*/
typedef int (*CMD_FUNC) (int argc, char *argv[]);

/*======================================================================
  command information struct
======================================================================*/
typedef struct command_information {
    char    *cmd_name;
    char    *cmd_desc;      /* short description for "help" command */
    CMD_FUNC cmd_op_func;   /* the operate function */
} CMD_INF;

/*======================================================================
  command line module support routines for other modules
======================================================================*/
void cmd_add(CMD_INF *new_cmd_info);
int  cmd_do(char *cmd_line);
int  cmd_show(int argc, char *argv[]);
int  cmd_tab(const char *prefix, char *cmd_line);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CMD_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

