/*==============================================================================
** str_filter.c -- string filter. it's filter strings like "", "*.avi", "a*b|c"
**
** MODIFY HISTORY:
**
** 2012-04-03 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include "../../string.h"

/*======================================================================
  string filter configs
======================================================================*/
#define STR_FILTER_MAX_LEN   40

/*======================================================================
  String Filter Struct
======================================================================*/
typedef struct str_filter {
    int  filter_len;

    char next_char[STR_FILTER_MAX_LEN];
    int  fail_back[STR_FILTER_MAX_LEN];

    struct str_filter *pNext;
} STR_FILTER;

/*==============================================================================
 * - str_filter_create()
 *
 * - create a string filter with <filter>
 */
STR_FILTER * str_filter_create (const char *filter)
{
    int i;
    int pos = 0;      /* the <next_char> arrary can store char position */  
    int back_to = -1; /* the last '*' postion */
	char *pSep;
    STR_FILTER *pFilter = calloc (1, sizeof (STR_FILTER));
    if (pFilter == NULL) {
        return NULL;
    }

    if ((pSep = strchr (filter, '|')) != NULL) {
        pFilter->filter_len = pSep - filter;
    } else {
        pFilter->filter_len = strlen (filter);
    }

    for (i = 0; i < pFilter->filter_len; i++) {
        if (filter[i] == '*') {
            back_to = pos;
        } else {
            pFilter->next_char[pos] = filter[i];
            pFilter->fail_back[pos++] = back_to;
        }

        if (pos == STR_FILTER_MAX_LEN - 1) {
            break;
        }
    }

    pFilter->next_char[pos] = 0;
    pFilter->fail_back[pos] = back_to;

    if (filter[i] == '|') {
        pFilter->pNext = str_filter_create (&filter[i+1]);
    }

    return pFilter;
}

/*==============================================================================
 * - str_filter_is_compatibility()
 *
 * - check whether the <string> compatibility to the <pFilter>
 */
BOOL str_filter_is_compatibility (const STR_FILTER *pFilter, const char *string)
{
    int i;
    int str_len;
    unsigned int pos = 0;;

    if (pFilter == NULL) {
        return FALSE;
    }
    if (pFilter->filter_len == 0) {
        return TRUE;
    }

    str_len = strlen (string);

    for (i = 0; i < str_len; i++) {
        if (string[i] == pFilter->next_char[pos]) { /* this char is crrect */
            pos++;
        } else {                                    /* this char not equal */
            pos = pFilter->fail_back[pos];
            if (pos == -1) { /* we back to a wrong feild */
                return str_filter_is_compatibility (pFilter->pNext, string);
            }
            if (string[i] == pFilter->next_char[pos]) {
                pos++;
            }
        }

    }
    
    if (pFilter->next_char[pos] == '\0') {
        return TRUE;
    } else {
        return str_filter_is_compatibility (pFilter->pNext, string);
    }
}

/*==============================================================================
 * - str_filter_delete()
 *
 * - delete a string filter
 */
void str_filter_delete (const STR_FILTER *pFilter)
{
    if (pFilter != NULL) {
        free ((void*)pFilter);
    }
}

/*==============================================================================
** file_mgr.c -- file magager.
**
** MODIFY HISTORY:
**
** 2012-04-03 wdf Create.
==============================================================================*/
#include <yaffs_guts.h>
#include "../gui.h"
#include "../LibJPEG/jpeg_util.h"
#include "file_mgr.h"
#include "msg_box.h"
extern BOOL yaffs_is_dir (const char *file_name);
extern void yaffs_up_dir (char *dir);

/*======================================================================
  File Manager Configs
======================================================================*/
#define FILE_MGR_MAX_FILES      256
#define DIR_ICON_NAME          "/n1/gui/cbi_dir.jpg"
#define FILE_ICON_NAME         "/n1/gui/cbi_file.jpg"

#define FILE_ICON_SIZE          64 /* "cbi_file.jpg" size */
#define FILES_PER_LAYER         8  /* per line shows file number */
#define FILE_AREA_START_X       0  /* first column start x coordinate */
#define FILE_AREA_START_Y       10 /* first row start y coordinate */
#define FILE_AREA_WIDTH         (gra_scr_w() - ICON_SIZE)
#define FILE_AREA_HEIGHT        (gra_scr_h() - FILE_AREA_START_Y)
#define BLANK_WIDTH             ((FILE_AREA_WIDTH - FILES_PER_LAYER * FILE_ICON_SIZE) / FILES_PER_LAYER)
#define ICON_NAME_MARGIN        4
#define ROW_ROW_MARGIN          8
#define BLANK_HEIGHT            (ICON_NAME_MARGIN + GUI_FONT_HEIGHT + ROW_ROW_MARGIN)

/*======================================================================
  File Manager Struct
======================================================================*/
typedef struct file_mgr {
    GUI_CBI    *file_cbis[FILE_MGR_MAX_FILES];
    int         file_num;
    
    char        pwd[PATH_LEN_MAX];
    char        filter[STR_FILTER_MAX_LEN];
} FILE_MGR;

/*======================================================================
  Current Show Files' CBIs
======================================================================*/
static FILE_MGR _G_file_mgr;
#define _G _G_file_mgr /* just for shorter */

static OS_STATUS   _file_mgr_open_dir (GUI_CBI *pCBI_dir, GUI_COOR *coor);
static CB_FUNC_PTR _get_open_func (const char *name);
static OS_STATUS   _default_open_func (GUI_CBI *pCBI_file, GUI_COOR *coor);

/*==============================================================================
 * - file_mgr_show()
 *
 * - show the files in <path> which compatibility <filter>
 */
int file_mgr_show (const char *path, const char *filter)
{
    yaffs_DIR     *d = NULL;
    yaffs_dirent  *de = NULL;
    STR_FILTER    *pFilter = NULL;
    int            i = 0;
    char           full_name[PATH_LEN_MAX];
    int 		   path_len;

    d = yaffs_opendir(path);

    /* create filter */
    pFilter = str_filter_create (filter);
    if (pFilter == NULL) {
        return i;
    }
    strcpy (_G.filter, filter); /* save filter */

    /* copy path to <pwd> */
    strcpy (_G.pwd, path); /* save current path */
    /* append a slash */
    if (_G.pwd[strlen(_G.pwd) - 1] != '/') {
        strcat(_G.pwd, "/");
    }
    path_len = strlen(_G.pwd);
    strcpy (full_name, _G.pwd);

    /* clear previous files */
    file_mgr_clear ();

    /* add files */
    if ((d == NULL) && (strcmp (_G.pwd, "/") == 0)) { /* if <_G.pwd> is root directory */
        _G.file_cbis[i++] = file_mgr_reg_one ("/boot", "boot", _file_mgr_open_dir);
        _G.file_cbis[i++] = file_mgr_reg_one ("/n1", "n1", _file_mgr_open_dir);
    } else { /* add the files & directories in <_G.pwd> */
        for (i = 0; (de = yaffs_readdir(d)) != NULL; i++) {
            strcpy (&full_name[path_len], de->d_name);
            if ((path_len + strlen (de->d_name)) < GUI_CBI_NAME_LEN_MAX) { /* file name not too long */
                if (yaffs_is_dir (full_name)) { /* this entry is a directory */
                    _G.file_cbis[i] = file_mgr_reg_one (full_name, de->d_name, _file_mgr_open_dir);
                } else if (str_filter_is_compatibility (pFilter, de->d_name)) { /* suit the filter */
                    _G.file_cbis[i] = file_mgr_reg_one (full_name, de->d_name, _get_open_func(de->d_name));
                } else {
                    i--;
                }
            } else {
                i--;
            }
        }
        yaffs_closedir(d);
    }

    _G.file_cbis[i] = NULL;
    str_filter_delete (pFilter);

    return i;
}

/*==============================================================================
 * - file_mgr_reg_one()
 *
 * - register and show a file cbi
 */
GUI_CBI *file_mgr_reg_one (const char *full_name, const char *name, CB_FUNC_PTR open_func)
{
    static GUI_COLOR *pDir_pic = NULL;
    static GUI_COLOR *pFile_pic = NULL;
    GUI_COOR file_icon_coor;
    GUI_COOR right_down;
    GUI_COOR file_name_coor;
    GUI_CBI *pCBI = NULL;
    GUI_SIZE size = {FILE_ICON_SIZE, FILE_ICON_SIZE};
    int name_show_num;
    const int col_width = BLANK_WIDTH + FILE_ICON_SIZE;
    const int row_height = BLANK_HEIGHT + FILE_ICON_SIZE;

    /* calculate current cbi layer & order in layer */
    int layer = _G.file_num / FILES_PER_LAYER;
    int order = _G.file_num % FILES_PER_LAYER;

    /* draw waring message if out of file area */
    if (layer >= FILE_AREA_HEIGHT / row_height) {
        GUI_COOR warning_coor = {FILE_AREA_START_X, FILE_AREA_START_Y + FILE_AREA_HEIGHT - GUI_FONT_HEIGHT};
        font_printf (&warning_coor, GUI_COLOR_RED, "There is not enough space to show files!");
        return NULL;
    }

    /* get icons pixels */
    if (pDir_pic == NULL) {
        pDir_pic = get_pic_pixel (DIR_ICON_NAME, NULL, NULL);
    }
    if (pFile_pic == NULL) {
        pFile_pic = get_pic_pixel (FILE_ICON_NAME, NULL, NULL);
    }

    /* register cbi */
    file_icon_coor.x = FILE_AREA_START_X + BLANK_WIDTH / 2 + order * col_width;
    file_icon_coor.y = FILE_AREA_START_Y + layer * row_height;
    right_down.x = file_icon_coor.x + FILE_ICON_SIZE - 1;
    right_down.y = file_icon_coor.y + FILE_ICON_SIZE - 1;
    pCBI = cbi_create_blank (&file_icon_coor, &right_down, cbf_default_press);
    if (open_func == _file_mgr_open_dir) {
        gra_block (&file_icon_coor, &size, pDir_pic);
    } else {
        gra_block (&file_icon_coor, &size, pFile_pic);
    }
    pCBI->func_release = open_func; 
    strcpy (pCBI->name, full_name); /* store full_name to cbi's name */

    /* draw file name */
    name_show_num = txt_get_line (name, col_width - GUI_FONT_WIDTH);
    file_name_coor.x = (file_icon_coor.x - BLANK_WIDTH / 2 + (col_width - name_show_num * GUI_FONT_WIDTH) / 2);
    file_name_coor.y = file_icon_coor.y + FILE_ICON_SIZE + ICON_NAME_MARGIN;
    han_draw_string (&file_name_coor, GUI_COLOR_WHITE, (uint8 *)name, name_show_num);

    /* inc total files num */
    _G.file_num++;

    return pCBI;
}

/*==============================================================================
 * - file_mgr_clear()
 *
 * - clear file area
 */
void file_mgr_clear ()
{
    int i = 0;
    GUI_COOR area_start = {FILE_AREA_START_X, FILE_AREA_START_Y};
    GUI_SIZE area_size = {FILE_AREA_WIDTH, FILE_AREA_HEIGHT};

    /* unregister file cbis */
    while (_G.file_cbis[i] != NULL) {
        cbi_unregister (_G.file_cbis[i]);
        i++;
    }

    /* clear file area */
    gra_rect (&area_start, &area_size, GUI_BG_COLOR);

    /* manager no file */
    _G.file_num = 0;
}

/*==============================================================================
 * - _file_mgr_open_dir()
 *
 * - open a directory, the callback of files in it is default
 */
static OS_STATUS _file_mgr_open_dir (GUI_CBI *pCBI_dir, GUI_COOR *coor)
{
    file_mgr_show (pCBI_dir->name, _G.filter);
    return OS_STATUS_OK;
}

/*==============================================================================
 * - file_mgr_fresh()
 *
 * - refresh current directory files use last <filter>
 */
OS_STATUS file_mgr_fresh ()
{
    file_mgr_show (_G.pwd, _G.filter);
    return OS_STATUS_OK;
}

/*==============================================================================
 * - file_mgr_updir()
 *
 * - up to current directory father layer
 */
OS_STATUS file_mgr_updir ()
{
    yaffs_DIR *d;
    char cur_dir[PATH_LEN_MAX];

    /* get new directory */
    strcpy (cur_dir, _G.pwd);
    yaffs_up_dir (cur_dir);

    /* check new directory is exist */
    d = yaffs_opendir(cur_dir);
    if (d) {
        yaffs_closedir(d);
    } else if (strcmp (cur_dir, "/") != 0) {
        char msg_str[PATH_LEN_MAX + 40];
        strcpy (msg_str, "directory '");
        strcat (msg_str, cur_dir);
        strcat (msg_str, "' not exist!");
        msg_box_create (msg_str);
        return CMD_ERROR;
    }

    strcpy(_G.pwd, cur_dir);
    file_mgr_show (_G.pwd, _G.filter);
    return OS_STATUS_OK;
}

/*======================================================================
  file open function manager struct
======================================================================*/
typedef struct exec_node {
    DL_NODE  file_open_func_node;

    char        filter[STR_FILTER_MAX_LEN];
    STR_FILTER *pFilter;
    CB_FUNC_PTR open_func;
} EXEC_NODE;

/*======================================================================
  list all register file open function nodes
======================================================================*/
static DL_LIST _G_exec_list = {NULL, NULL};

/*==============================================================================
 * - _get_open_func()
 *
 * - get the file open function from the list
 */
static CB_FUNC_PTR _get_open_func (const char *name)
{
	CB_FUNC_PTR   open_func = _default_open_func;
    EXEC_NODE *pExec = (EXEC_NODE *)DL_FIRST (&_G_exec_list);

    while (pExec != NULL) {

        /* check whether suit the filter */
        if (str_filter_is_compatibility (pExec->pFilter, name)) {
            open_func = pExec->open_func;
            break;
        }

        pExec = (EXEC_NODE *)DL_NEXT (pExec);
    }

    return open_func;
}

/*==============================================================================
 * - _default_open_func()
 *
 * - when user want to open a file that have not register open function,
 *   call this
 */
static OS_STATUS _default_open_func (GUI_CBI *pCBI_file, GUI_COOR *coor)
{
    char msg_str[PATH_LEN_MAX + 20];

    cbf_default_release (pCBI_file, coor);

    strcpy (msg_str, "\nThere is no routine\n to open file :\n\n");
    strcat (msg_str, pCBI_file->name);
    msg_box_create (msg_str);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - file_mgr_register()
 *
 * - register an open function to the files those compatibility to <filter>
 */
OS_STATUS file_mgr_register (const char *filter,  CB_FUNC_PTR file_open_func)
{
    EXEC_NODE *pExec = (EXEC_NODE *)DL_FIRST (&_G_exec_list);

    /* try to find already register one */
    while (pExec != NULL) {
        if (strcmp (filter, pExec->filter) == 0) {
            pExec->open_func = file_open_func;
            return OS_STATUS_OK;
        }
        pExec = (EXEC_NODE *)DL_NEXT (pExec);
    }

    /* alloc a new execute noe */
    pExec = malloc (sizeof(EXEC_NODE));
    if (pExec == NULL) {
        return OS_STATUS_ERROR;
    }

    /* create filter */
    pExec->pFilter = str_filter_create (filter);
    if (pExec->pFilter == NULL) {
        free(pExec);
        return OS_STATUS_ERROR;
    }
    strcpy (pExec->filter, filter); /* save filter */

    /* store open function */
    pExec->open_func = file_open_func;

    /* add the new node to list */
    dlist_insert (&_G_exec_list, NULL, (DL_NODE *)pExec); /* insert head */

    return OS_STATUS_OK;
}

/*==============================================================================
 * - file_mgr_unregister()
 *
 * - unregister an open function to the files those compatibility to <filter>
 */
OS_STATUS file_mgr_unregister (const char *filter)
{
    EXEC_NODE *pExec = (EXEC_NODE *)DL_FIRST (&_G_exec_list);

    while (pExec != NULL) {
        if (strcmp (filter, pExec->filter) == 0) {
            break;
        }
        pExec = (EXEC_NODE *)DL_NEXT (pExec);
    }

    if (pExec != NULL) {
        dlist_remove (&_G_exec_list, (DL_NODE *)pExec);
        str_filter_delete (pExec->pFilter);
        free (pExec);
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - file_mgr_unregister_all()
 *
 * - unregister all open function
 */
OS_STATUS file_mgr_unregister_all ()
{
    EXEC_NODE *pExec = (EXEC_NODE *)dlist_get (&_G_exec_list);

    while (pExec != NULL) {
        str_filter_delete (pExec->pFilter);
        free (pExec);
        pExec = (EXEC_NODE *)dlist_get (&_G_exec_list);
    }

    return OS_STATUS_OK;
}

/*==============================================================================
** FILE END
==============================================================================*/

