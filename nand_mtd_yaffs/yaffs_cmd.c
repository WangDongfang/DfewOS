/*==============================================================================
** yaffs_cmd.c -- yaffs command module.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/
#include "yaffs_guts.h"

#include "../config.h"
#include "../types.h"
#include "../dlist.h"
#include "../string.h"
#include "../cmd.h"

/*======================================================================
  max absolutely path name length
======================================================================*/
#define NAME_WIDTH      20
#define NAMES_PER_LINE  4

/*======================================================================
  current directory, init as "/n1/"
======================================================================*/
static char _G_pwd[PATH_LEN_MAX] = "/n1/";

/*======================================================================
  detect error macro 
======================================================================*/
#define CHECK_ARG_NUM(min_num, tip)  \
    if (argc < min_num) { \
        serial_printf(tip);\
        return CMD_ERROR;   \
    }

#define CHECK_YAFFS_RETVAL(retval, tip_fmt, tip_arg1, tip_arg2)  \
    if (retval < 0) { \
        serial_printf(tip_fmt, tip_arg1, tip_arg2); \
        serial_printf("yaffs error: %d", retval);  \
        return CMD_ERROR;   \
    }

/*======================================================================
  forward functions declare
======================================================================*/
static char  *_make_abs_path (char *abs_path, const char *user_path);
static void   _dir_append (char *old_dir, const char *append);
BOOL yaffs_is_dir (const char *file_name);
BOOL yaffs_is_dir2 (const char *dir, const char *file_name);
void yaffs_up_dir (char *dir);

/*==============================================================================
 * - ls()
 *
 * - list the files and direcories in a directory
 */
int ls(int argc, char *argv[])
{
    int i, j;
    char dir[PATH_LEN_MAX];

    yaffs_DIR *d;
    yaffs_dirent *de;

    if (argc < 2) {
        strcpy(dir, _G_pwd);
    } else {
        _make_abs_path(dir, argv[1]);
    }

    d = yaffs_opendir(dir);

    if(!d) {
        serial_printf("opendir failed");
        return CMD_ERROR;
    } else {
        for(i = 0; (de = yaffs_readdir(d)) != NULL; i++) {
            if ((i != 0) && (i % NAMES_PER_LINE) == 0 ) {
                serial_printf("\n");
            }
            if (yaffs_is_dir2 (dir, de->d_name)) {
                serial_printf (COLOR_FG_GREEN);
            } else {
                serial_printf (COLOR_FG_WHITE);
            }
            serial_printf ("%s", de->d_name);
            for (j = strlen (de->d_name); j < NAME_WIDTH; j++) {
                serial_putc (' ');
            }
        }
        serial_printf (COLOR_FG_WHITE);
    }
    yaffs_closedir(d);

    return CMD_OK;
}

/*==============================================================================
 * - cd()
 *
 * - change directory
 */
int cd(int argc, char *argv[])
{
    yaffs_DIR *d;
    char temp_dir[PATH_LEN_MAX];

    CHECK_ARG_NUM(2, "please type dir name");

    /*
     * get the new dir 
     */
    _make_abs_path(temp_dir, argv[1]);

    /*
     * try to open the new dir
     */
    d = yaffs_opendir(temp_dir);

    if (!d) {
        serial_printf("directory '%s' not exist", temp_dir);
        return CMD_ERROR;
    } else {
        strcpy(_G_pwd, temp_dir);
    }
    yaffs_closedir(d);

    /*
     * append a slash
     */
    if (_G_pwd[strlen(_G_pwd) - 1] != '/') {
        strcat(_G_pwd, "/");
    }

    return CMD_OK;
}

/*==============================================================================
 * - pwd()
 *
 * - show current directory
 */
int pwd(int argc, char *argv[])
{
    serial_printf(_G_pwd);
    return CMD_OK;
}

/*==============================================================================
 * - mv()
 *
 * - move one file from some loacte to another
 */
int mv(int argc, char *argv[])
{
    int yaffs_retval;

    char old_path_name[PATH_LEN_MAX];
    char new_path_name[PATH_LEN_MAX];

    CHECK_ARG_NUM(3, "too few argument");

    _make_abs_path(old_path_name, argv[1]);
    _make_abs_path(new_path_name, argv[2]);

    yaffs_retval = yaffs_rename(old_path_name, new_path_name);

    CHECK_YAFFS_RETVAL(yaffs_retval, "can't move '%s' to '%s'\n",
                       old_path_name, new_path_name);

    return CMD_OK;
}

/*==============================================================================
 * - rm()
 *
 * - delete a file
 */
int rm(int argc, char *argv[])
{
    int yaffs_retval;
    char path_name[PATH_LEN_MAX];

    CHECK_ARG_NUM(2, "please type file name");
    _make_abs_path(path_name, argv[1]);

    yaffs_retval = yaffs_unlink(path_name);

    CHECK_YAFFS_RETVAL(yaffs_retval, "can't delete '%s'%s\n", path_name, "");

    return CMD_OK;
}

/*==============================================================================
 * - rmdir()
 *
 * - delete a directory
 */
int rmdir(int argc, char *argv[])
{
    int yaffs_retval;
    char path[PATH_LEN_MAX];

    CHECK_ARG_NUM(2, "please type file name");
    _make_abs_path(path, argv[1]);

    yaffs_retval = yaffs_rmdir(path);

    CHECK_YAFFS_RETVAL(yaffs_retval, "can't delete '%s'%s\n", path, "");

    return CMD_OK;
}

/*==============================================================================
 * - cp()
 *
 * - copy a file form here to there
 */
int cp(int argc, char *argv[])
{
    int fd_in, fd_out;
    int read_byte, write_byte;
    char file_context[1024];

    char old_path_name[PATH_LEN_MAX];
    char new_path_name[PATH_LEN_MAX];

    CHECK_ARG_NUM(3, "too few argument");

    _make_abs_path(old_path_name, argv[1]);
    fd_in = yaffs_open(old_path_name, O_RDONLY, 0);
    CHECK_YAFFS_RETVAL(fd_in, "can't open file '%s'%s\n", old_path_name, "");

    _make_abs_path(new_path_name, argv[2]);
    fd_out = yaffs_open(new_path_name, O_CREAT|O_RDWR|O_TRUNC, S_IREAD|S_IWRITE);
    if (fd_out == -1) {
        yaffs_close(fd_in);
        serial_printf("can't create file '%s'", new_path_name);
        return CMD_ERROR;
    }

    /*
     * read from fd_in and write to fd_out
     */
    read_byte = yaffs_read(fd_in, file_context, 1024);
    while (read_byte > 0) {
        write_byte = yaffs_write(fd_out, file_context, read_byte);
        if (write_byte != read_byte) {
            serial_printf("copy file failed!");
            break;
        }

        read_byte = yaffs_read(fd_in, file_context, 1024);
    }

    yaffs_close(fd_in);
    yaffs_close(fd_out);

    if (read_byte > 0) {
        return CMD_ERROR;
    } else {
        return CMD_OK;
    }
}

/*==============================================================================
 * - y_mkdir()
 *
 * - make a new directory
 */
int y_mkdir(int argc, char *argv[])
{
    int yaffs_retval;
    char path[PATH_LEN_MAX];

    CHECK_ARG_NUM(2, "please type directory name");

    _make_abs_path(path, argv[1]);

    yaffs_retval = yaffs_mkdir(path, 0);

    CHECK_YAFFS_RETVAL(yaffs_retval, "can't make directory '%s'%s\n", path, ""); 

    return CMD_OK;
}

/*==============================================================================
 * - cat()
 *
 * - print a file context
 */
int cat(int argc, char *argv[])
{
    int fd;
    int read_byte;
    char file_context[1024];
    char path_name[PATH_LEN_MAX];

    CHECK_ARG_NUM(2, "please type file name");

    _make_abs_path(path_name, argv[1]);

    /*
     * open file
     */
    fd = yaffs_open(path_name, O_RDONLY, 0);
    if ( fd ==  -1) {
        serial_printf("cannot open file '%s'", path_name);
        return CMD_ERROR;
    }

    /*
     * read and print file context
     */
    read_byte = yaffs_read(fd, file_context, 1023);
    while (read_byte > 0) {
        file_context[read_byte] = '\0';
        serial_printf(file_context);

        read_byte = yaffs_read(fd, file_context, 1023);
    }

    yaffs_close(fd);
    return CMD_OK;
}

/*==============================================================================
 * - yaffs_is_dir()
 *
 * - check whether directory. <file_name> is full name
 */
BOOL yaffs_is_dir (const char *file_name)
{
    struct yaffs_stat   statFile;

    if (yaffs_stat (file_name, &statFile) < 0) { /* not exist */
        return FALSE;
    }

    return S_ISDIR(statFile.st_mode) ? TRUE : FALSE;
}

/*==============================================================================
 * - yaffs_is_dir2()
 *
 * - check whether directory. <dir> must be not empty
 */
BOOL yaffs_is_dir2 (const char *dir, const char *file_name)
{
    char full_name[PATH_LEN_MAX];

    strcpy (full_name, dir);

    /* append a slash if need */
    if (full_name[strlen(full_name) - 1] != '/') {
        strcat(full_name, "/");
    }

    strcat (full_name, file_name);

    return yaffs_is_dir (full_name);
}

/*==============================================================================
 * - yaffs_up_dir()
 *
 * - go to father directory. the <dir> type is "/sdf/123/"
 */
void yaffs_up_dir (char *dir)
{
    char *end;
    int len = strlen(dir);
    if (len <= 1) {
        return;
    }
    end = dir + len - 1; /* end point the last '/' */

    /*
     * until end point to prev '/'
     */
    while (*--end != '/')
        ;

    *++end = '\0';
}

/*======================================================================
  Directory Files List Node
======================================================================*/
typedef struct dir_list_node {
    DL_NODE  dir_list_node;
    char     name[PATH_LEN_MAX];
} DIR_LIST_NODE;

/*======================================================================
  Current Directory Files List
======================================================================*/
static DL_LIST _G_dir_list = {NULL, NULL};

/*==============================================================================
 * - _dispatch_name()
 *
 * - dispatch the path and file name, like this:
 *   "/n1/safe_" --> "/n1/", "safe_"
 *   "/n1/gui/" --> "/n1/gui/", ""
 */
static void _dispatch_name (char *path, char *name)
{
    char *end;
    int len = strlen(path);
    end = path + len - 1; /* end point the last '/' */

    /*
     * until end point to before '/'
     */
    while (*end-- != '/')
        ;

    end += 2;
 
    *name++ = *end;
    *end++ = '\0';

    while (*end != '\0') {
        *name++ = *end++;
    }

    *name = '\0';
}

/*==============================================================================
 * - _list_dir()
 *
 * - refresh the directory list nodes
 */
static int _list_dir (const char *path, DL_LIST *pDirList)
{
    int            i;
    yaffs_DIR     *d;
    yaffs_dirent  *de;
    DIR_LIST_NODE *pListNode = NULL;
    char           full_name[PATH_LEN_MAX];

    d = yaffs_opendir(path);

    /* free previous list nodes */
    pListNode = (DIR_LIST_NODE *)dlist_get (pDirList);
    while (pListNode != NULL) {
        free (pListNode);
        pListNode = (DIR_LIST_NODE *)dlist_get (&_G_dir_list);
    }

    if (!d) {
        return 0;
    }

    /* alloc new list nodes */
    for(i = 0; (de = yaffs_readdir(d)) != NULL; i++) {
        pListNode = malloc (sizeof (DIR_LIST_NODE));
        if (pListNode != NULL) {
            strcpy(pListNode->name, de->d_name);

            strcpy(full_name, path);
            strcat(full_name, de->d_name);
            if (yaffs_is_dir (full_name)) {
                strcat(pListNode->name, "/");
            }

            dlist_add (pDirList, (DL_NODE *)pListNode);
        }
    }
    yaffs_closedir(d);

    return i;
}

/*==============================================================================
 * - yaffs_tab()
 *
 * - auto complete the command with <prefix> initial, store in <cmd_line>
 */
int yaffs_tab (const char *prefix, char *cmd_line)
{
    static char      last_prefix[PATH_LEN_MAX] = "";
    static char      last_path[PATH_LEN_MAX] = "";
    static DL_NODE  *pLast = NULL;
    char             abs_path[PATH_LEN_MAX];
    char             file_name[20];
    char            *pSlash = strrchr (prefix, '/');


    /* get path and name */
    _make_abs_path(abs_path, prefix);       /* get <abs_path> */
    _dispatch_name(abs_path, file_name);    /* get <path>, <name> */

    if (strcmp (last_prefix, prefix) != 0) { /* if user change prefix */
        strcpy (last_prefix, prefix);
        if (strcmp (last_path, abs_path) != 0) {
            strcpy (last_path, abs_path);
            _list_dir (abs_path, &_G_dir_list);
        }
        pLast = (DL_NODE *)DL_FIRST(&_G_dir_list);
    } else { /* use last prefix */
        if (pLast == NULL) {
            pLast = (DL_NODE *)DL_FIRST(&_G_dir_list);
        } else {
        	pLast = (DL_NODE *)DL_NEXT(pLast);
        }
    }

    /* store path */
    if (pSlash != NULL) { /* there is some path */
        strlcpy (cmd_line, prefix, pSlash - prefix + 2);
    } else {
        cmd_line[0] = '\0';
    }

    /* find matched file name */
    while (pLast != NULL) {
        if (strncmp (file_name, ((DIR_LIST_NODE *)pLast)->name, strlen(file_name)) == 0) {
            strcat (cmd_line, ((DIR_LIST_NODE *)pLast)->name);
            break;
        }
        pLast = (DL_NODE *)DL_NEXT(pLast);
    }

    if (pLast == NULL) {
        strcat (cmd_line, file_name);
    }

    return strlen (cmd_line);
}


/*==============================================================================
 * - _make_abs_path()
 *
 * - convert to absolutely path
 */
static char  *_make_abs_path (char *abs_path, const char *user_path)
{
    if (user_path[0] == '/') {    /* absolutely path */
        strcpy(abs_path, user_path);
    } else {                    /* relatively path */
        strcpy(abs_path, _G_pwd);
        _dir_append(abs_path, user_path);
    }

    return abs_path;
}

/*==============================================================================
 * - _dir_append()
 *
 * - append a path to the old path. <old_path> end with '/'
 */
static void _dir_append (char *old_dir, const char *append)
{
    char *slash_p;
    slash_p = strchr (append, '/');   /* find '/' postion */

    if (slash_p == NULL) {    /* there isn't '/' char, this is the last */

        if (!strcmp(append, ".")) {
            /* NOOP */
        } else if (!strcmp(append, "..")) {
            yaffs_up_dir(old_dir);
        } else {
            strcat(old_dir, append);
        }
        return;
    } 

    if (((slash_p - append) == 1) && (append[0] == '.')) {   /* like "./asdf/sdf" */
        /* NOOP */
    } else if(((slash_p - append) == 2) && 
              (!strncmp(append, "..", 2))) {                 /* like "../asdf/sdf" */
        yaffs_up_dir(old_dir);
    } else {                                                 /* like "asd/sdf" */
        /* just append "asd/" */
        strncat(old_dir, append, (slash_p - append + 1));
    }

    append = slash_p + 1; /* next item */
    _dir_append(old_dir, append);
}

/*==============================================================================
 * - my_yaffs_open()
 *
 * - open a relatively or absolutely path file
 */
int my_yaffs_open(const char *file_name, int oflag, int mode)
{
    char path_name[PATH_LEN_MAX];
    _make_abs_path(path_name, file_name);
    return yaffs_open(path_name, oflag, mode);
}

/*==============================================================================
 * - my_yaffs_stat()
 *
 * - check a relatively or absolutely file stat
 */
int my_yaffs_stat(const char *file_name, struct yaffs_stat *stat_buf)
{
    char path_name[PATH_LEN_MAX];
    _make_abs_path(path_name, file_name);
    return yaffs_stat(path_name, stat_buf);
}

/**********************************************************************************************************
  init the yaffs commands, such as "ls" "cd" "pwd"
**********************************************************************************************************/
static CMD_INF _G_yaffs_cmds[] = {
    {"ls",   "list files in directory", ls},
    {"cd",   "change directory",        cd},
    {"pwd",  "show current directory",  pwd},
    {"mv",   "move one file to other position",   mv},
    {"rm",   "delete a file",   rm},
    {"rmdir","delete a directory",   rmdir},
    {"cp",   "copy one file to other position",   cp},
    {"mkdir","make a directory",   y_mkdir},
    {"cat",  "show a text file context",   cat}
};

void yaffs_cmd_init()
{
    int i;
    int cmd_num;
    cmd_num = N_ELEMENTS(_G_yaffs_cmds);

    for (i = 0; i < cmd_num; i++) {
        cmd_add(&_G_yaffs_cmds[i]);
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

