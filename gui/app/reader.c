/*==============================================================================
** reader.c -- Text reader.
**
** MODIFY HISTORY:
**
** 2011-12-16 wdf Create.
==============================================================================*/
/*
                          +------------------+
                          |                  |
  +--------+              V                  |
  |        |        +------------+           |
  |  HOME  |<-------| list page  |------+    |
  |        |        +------------+      |    |
  +--------+              click file cbi|    |
                    +------------+      |    |
             +------|context page|<-----+    |
             |      +------------+           |
             |         A      A              |
        drag |         |      |              |
         to  |    begin|      |end cbi       |list cbi
        left |     cbi |      |              |
             |         |      |              |
             |      +------------+           |
             +----->|control page|-----------+
                    +------------+

The start is "list page".
*/

#include <dfewos.h>
#include <yaffs_guts.h>
#include "../gui.h"
#include "../driver/lcd.h"
#include "../LibJPEG/jpeg_util.h"

/*======================================================================
  configs
======================================================================*/
#define READER_FILE_PATH        "/n1/reader/"
#define READER_FILE_POS_FILE    READER_FILE_PATH".file_pos"
#define FILE_ICON_NAME          "/n1/gui/cbi_file.jpg"

#define FILE_ICON_SIZE          64 /* cbi_file.jpg size */
#define FILE_ICON_START_X       10 /* first column start x coordinate */
#define FILE_ICON_START_Y       10 /* first row start y coordinate */
#define FILE_ICON_INTV          20 /* the gap between two row */

#define READER_BUFFER_SIZE     (4 * KB) /* file context buffer */
#define MAX_FILES_POS           50 /* the max entry num of file pos */
#define SLIDE_DELAY             40000 /* slide the control bar delay */

#define READER_TEXT_COLOR       GUI_COLOR_YELLOW

/*======================================================================
  constant
======================================================================*/
#define CONTROL_CBI_NUM         5 /* control bar cbi num */

#define ALARM_OFFSET            320
#define UP_ALARM_LEVEL          (0)
#define LOW_ALARM_LEVEL         ((gra_scr_w() * 2) - ALARM_OFFSET)
 
#define DRAG_NOOP               0
#define DRAG_SLIDE              1
#define DRAG_CONTEXT            2

/*======================================================================
  Global variables
======================================================================*/
/* file position array */
typedef struct txt_file_position {
    char     name[PATH_LEN_MAX];

    int      start_byte;
    int      fb_cur_x;
} TXT_FILE_POS;
static TXT_FILE_POS _G_files_pos[MAX_FILES_POS + 1];

/* file name list */
typedef struct txt_file_node {
    DL_NODE  file_list_node;
    char     name[PATH_LEN_MAX];
} TXT_FILE_NODE;
static DL_LIST _G_file_list = {NULL, NULL};

/* file context infomation */
typedef struct context_control {
    int  fd;
    int  file_pos_index;

    int  file_start_byte;
    int  fb_cur_x;

    int  scr0_bytes;
    int  scr1_bytes;
    int  scr2_bytes;

    int  is_reach_end; /* [0, 1] */

    int        slide_pos; /* [-1, 479] last row pos */
    GUI_COLOR *slide_context;
    GUI_COLOR *slide_control;

    int  drag_slide_or_context; /* [0, 1, 2] */
    int  auto_mode;
} CON_CTRL;
static CON_CTRL _G_current_context_control;
#define  _Gc _G_current_context_control

/*======================================================================
  Function foreward declare
======================================================================*/
/* list page cbi callback and help routines */
static int  _reader_add_files (const char *path, const char *ext);
static void _reader_reg_all_file_cbis ();
static void _reader_reg_one_file_cbi (TXT_FILE_NODE *p_file_node, const GUI_COOR *p_coor);
static OS_STATUS _reader_cb_home (GUI_CBI *pCBI_home, GUI_COOR *coor);
static OS_STATUS _reader_cb_read (GUI_CBI *pCBI_file, GUI_COOR *coor);

static void _reader_file_pos_restore ();
static void _reader_file_pos_store ();
static int  _reader_file_pos_find (char *file_name);

/* context page cbi callback and help routines */
static int  _reader_read_a_screen (int which_scr, int read_start);
static void _reader_reg_context_cbi ();
static OS_STATUS _reader_cb_context_drag (GUI_CBI *pCBI_context, GUI_COOR *p_cbi_coor);
static OS_STATUS _reader_cb_context_release (GUI_CBI *pCBI_context, GUI_COOR *p_coor);
static OS_STATUS _reader_slide_drag (GUI_CBI *pCBI_slide, GUI_COOR *p_offset_coor);
static OS_STATUS _reader_slide_release (GUI_CBI *pCBI_slide, GUI_COOR *coor);

/* control page cbi callback and help routines */
static void _reader_dump_control_icons ();
static void _reader_reg_control_cbis ();
static OS_STATUS _reader_cb_ctrl_hide (GUI_CBI *pCBI_hide, GUI_COOR *coor);
static OS_STATUS _reader_cb_ctrl_begin (GUI_CBI *pCBI_begin, GUI_COOR *coor);
static OS_STATUS _reader_cb_ctrl_end (GUI_CBI *pCBI_end, GUI_COOR *coor);
static OS_STATUS _reader_cb_ctrl_list (GUI_CBI *pCBI_list, GUI_COOR *coor);

/*==============================================================================
 * - app_reader()
 *
 * - reader application entry, called by home app
 */
OS_STATUS app_reader (GUI_CBI *pCBI_reader, GUI_COOR *coor)
{
    int i;

    ICON_CBI ic_right[] = {
        {"/n1/gui/cbi_home.jpg",     _reader_cb_home}
    };

    /*
     * clear home app bequest
     */
    cbi_delete_all (); /* delete all other cbis */
    gra_clear (GUI_BG_COLOR); /* clear screen */
    gra_set_show_fb (0);

    /* 
     * register 'home' cbi 
     */
    for (i = 0; i < N_ELEMENTS(ic_right); i++) {
        GUI_CBI *pCBI;
        GUI_COOR left_up = {gra_scr_w() - ICON_SIZE, i * ICON_SIZE * 4};
        GUI_SIZE size = {0, 0};

        pCBI = cbi_create_default (ic_right[i].name, &left_up, &size, FALSE);

        if (ic_right[i].func == _reader_cb_home) {
            pCBI->func_release = ic_right[i].func; /* home */
        } else {
            pCBI->func_press = ic_right[i].func;   /* for extern */
        }

        cbi_register (pCBI);
    }

    /*
     * get files those are in [READER_FILE_PATH], init <_G_file_list>,
     * and register all text file cbis,
     * user click one of these cbis, start read it
     */
    _reader_reg_all_file_cbis ();

    /*
     * read file position, init <_G_files_pos>
     */
    _reader_file_pos_restore ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_cb_home()
 *
 * - quit reader application
 */
static OS_STATUS _reader_cb_home (GUI_CBI *pCBI_home, GUI_COOR *coor)
{
    TXT_FILE_NODE *p_file_node = NULL;

    /* 
     * free file node memory
     */
    p_file_node = (TXT_FILE_NODE *)dlist_get (&_G_file_list);
    while (p_file_node != NULL) {
        free (p_file_node);
        p_file_node = (TXT_FILE_NODE *)dlist_get (&_G_file_list);
    }

    /*
     * store file postion file, write <_G_files_pos>
     */
    _reader_file_pos_store ();

    /*
     * go to home
     */
    gra_clear (GUI_BG_COLOR);
    cbf_go_home (pCBI_home, coor);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_file_pos_restore()
 *
 * - read file position file: [READER_FILE_POS_FILE], init <_G_files_pos>
 */
static void _reader_file_pos_restore ()
{
    int fd;
    int read_byte;
    GUI_COOR msg_coor = {0, 0};

    /* already read */
    if (_G_files_pos[0].name[0] != '\0') return;

    /* open */
    fd = yaffs_open (READER_FILE_POS_FILE, O_RDONLY, 0);
    if (fd < 0) {
        font_printf (&msg_coor, GUI_COLOR_RED, "Open \"%s\" failed!", 
                     READER_FILE_POS_FILE);
        return;
    }

    /* read */
    read_byte = yaffs_read (fd, &_G_files_pos[0], sizeof (TXT_FILE_POS) * MAX_FILES_POS);
    if (read_byte < 0) {
        return;
    }

    /* set a end mark */
    _G_files_pos[read_byte / sizeof (TXT_FILE_POS)].name[0] = '\0';

    /* close */
    yaffs_close (fd);
}

/*==============================================================================
 * - _reader_file_pos_store()
 *
 * - write <_G_files_pos> to [READER_FILE_POS_FILE], 
 */
static void _reader_file_pos_store ()
{
    int fd;
    int entry_num = 0;

    /* open */
    /* if mode is 0, the create file can't be readed */
    /* so here we use mode 0x666 */
    fd = yaffs_open (READER_FILE_POS_FILE, O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        return;
    }

    /* calcaute how many entry to write */
    while (_G_files_pos[entry_num].name[0] != '\0') {
        entry_num++;
    }

    /* write */
    yaffs_write (fd, &_G_files_pos[0], sizeof (TXT_FILE_POS) * entry_num);

    /* close */
    yaffs_close (fd);

    /* sign that postion file is wrote */
    _G_files_pos[0].name[0] = '\0';
}

/*==============================================================================
 * - _reader_file_pos_find()
 *
 * - get the file postion index in <_G_files_pos>
 */
static int _reader_file_pos_find (char *file_name)
{
    int i = 0;

    while (_G_files_pos[i].name[0] != '\0') {

        /* it already exist */
        if (strcmp (_G_files_pos[i].name, file_name) == 0) {
            return i;
        }

        i++;
    }

    /* can't find it, and the array is full */
    if (i == MAX_FILES_POS) {
        memcpy (&_G_files_pos[0], &_G_files_pos[1],
                sizeof (TXT_FILE_POS) * (MAX_FILES_POS - 1));
        i--;
    }

    /* init the new entry */
    strcpy (_G_files_pos[i].name, file_name);
    _G_files_pos[i].start_byte = 0;
    _G_files_pos[i].fb_cur_x   = 0;

    /* set the end mark */
    _G_files_pos[i + 1].name[0] = '\0';

    return i;
}

/*==============================================================================
 * - _reader_add_files()
 *
 * - search all file in <path> dir, and add extern with <ext> to list
 */
static int _reader_add_files (const char *path, const char *ext)
{
    yaffs_DIR     *d;
    yaffs_dirent  *de;

    int            i;
    TXT_FILE_NODE *p_file_node = NULL;
    int            file_name_len;

    d = yaffs_opendir(path);

    for(i = 0; (de = yaffs_readdir(d)) != NULL; i++) {

        file_name_len = strlen (de->d_name);

        if (strlen(path) +  file_name_len < PATH_LEN_MAX && /* file name not too long */
            strcmp (de->d_name + file_name_len - strlen(ext), ext) == 0) {

            p_file_node = calloc (1, sizeof (TXT_FILE_NODE));
            if (p_file_node != NULL) {
                strcpy(p_file_node->name, path);
                strcat(p_file_node->name, de->d_name);

                dlist_add (&_G_file_list, (DL_NODE *)p_file_node);
            }
        }
    }

    yaffs_closedir(d);

    return i;
}

/*==============================================================================
 * - _reader_reg_all_file_cbis()
 *
 * - register all files which are in [READER_FILE_PATH] directory
 */
static void _reader_reg_all_file_cbis ()
{
    TXT_FILE_NODE *p_file_node = NULL;
    GUI_COOR       coor = {FILE_ICON_START_X, FILE_ICON_START_Y};
    int            max_name_len = 0;
    int            name_len;

    _reader_add_files (READER_FILE_PATH, "");

    p_file_node = (TXT_FILE_NODE *)DL_FIRST(&_G_file_list);
    while (p_file_node != NULL) {
    	_reader_reg_one_file_cbi (p_file_node, &coor);

        name_len = strlen(p_file_node->name);
        max_name_len = MAX(max_name_len, name_len);

        coor.y += FILE_ICON_SIZE + FILE_ICON_INTV;
        if (coor.y + FILE_ICON_INTV > gra_scr_h()) {
            coor.y = FILE_ICON_START_Y;
            coor.x += FILE_ICON_SIZE +
                      (max_name_len - strlen (READER_FILE_PATH))* GUI_FONT_WIDTH;
            max_name_len = 0;
        }

        p_file_node = (TXT_FILE_NODE *)DL_NEXT(p_file_node);
    }
}

/*==============================================================================
 * - _reader_reg_one_file_cbi()
 *
 * - show and register a file cbi
 */
static void _reader_reg_one_file_cbi (TXT_FILE_NODE *p_file_node, const GUI_COOR *p_coor)
{
    GUI_SIZE size = {0, 0};
    GUI_CBI *pCBI = NULL;
    GUI_COOR name_coor;
    char    *file_name;
    
    /* register cbi */
    pCBI = cbi_create_default (FILE_ICON_NAME, p_coor, &size, FALSE);
    strlcpy (pCBI->name, p_file_node->name, GUI_CBI_NAME_LEN_MAX);
    pCBI->func_release = _reader_cb_read; 
    //pCBI->data = (uint8 *)p_file_node;
    cbi_register (pCBI);

    /* draw file name */
    name_coor.x = p_coor->x + FILE_ICON_SIZE;
    name_coor.y = p_coor->y + (FILE_ICON_SIZE - GUI_FONT_HEIGHT)/2;
    file_name = pCBI->name + strlen (READER_FILE_PATH);
    han_draw_string (&name_coor, GUI_COLOR_WHITE,
                     (uint8 *)file_name,
                     strlen (file_name));
}

/*==============================================================================
 * - _reader_cb_read()
 *
 * - start read a file
 */
static OS_STATUS _reader_cb_read (GUI_CBI *pCBI_file, GUI_COOR *coor)
{
    GUI_COOR msg_coor = {0, 0};
    
    /* open file */
    _Gc.fd = yaffs_open(pCBI_file->name, O_RDONLY, 0);
    if (_Gc.fd ==  -1) {
        font_printf (&msg_coor, GUI_COLOR_RED, "Open \"%s\" failed!",
                     pCBI_file->name);
        return OS_STATUS_ERROR;
    }

    /* init position */
    _Gc.file_pos_index = _reader_file_pos_find (pCBI_file->name);
    _Gc.file_start_byte = _G_files_pos[_Gc.file_pos_index].start_byte;
    _Gc.fb_cur_x        = _G_files_pos[_Gc.file_pos_index].fb_cur_x;

    _Gc.is_reach_end = 0;

    /* draw and screen 0, 1, 2 */
    _Gc.scr0_bytes =  _reader_read_a_screen (0, _Gc.file_start_byte);
    _Gc.scr1_bytes =  _reader_read_a_screen (1, _Gc.file_start_byte + _Gc.scr0_bytes);
    _Gc.scr2_bytes =  _reader_read_a_screen (2,
                               _Gc.file_start_byte + _Gc.scr0_bytes + _Gc.scr1_bytes);

    if (_Gc.scr2_bytes == 0) {
        if (_Gc.scr1_bytes != 0) {
            _Gc.fb_cur_x = MIN(_Gc.fb_cur_x, gra_scr_w());
        } else {
            _Gc.fb_cur_x = MIN(_Gc.fb_cur_x, 0);
        }
    }

    /* slide */
    _Gc.slide_pos = -1;
    _reader_dump_control_icons ();

    _Gc.drag_slide_or_context = DRAG_NOOP;

    /* move show fb */
    lcd_fb_move (-_Gc.fb_cur_x, 0);


    cbi_delete_all (); /* delete list page cbis */

    /* register context cbi */
    _reader_reg_context_cbi ();

    /* set to manual mode */
    _Gc.auto_mode = 0;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_read_a_screen()
 *
 * - read from file and draw on a screen
 */
static int _reader_read_a_screen (int which_scr, int read_start)
{
    int    i = 0;
    int    read_byte;
    uint8  read_buffer[READER_BUFFER_SIZE];
    GUI_COOR txt_coor = {which_scr * gra_scr_w(), gra_scr_h() - 1};
    int      txt_width;
    GUI_COOR blank_coor;
    GUI_SIZE blank_size;

    /* read context to buffer */
    read_byte = yaffs_pread(_Gc.fd, read_buffer, READER_BUFFER_SIZE, read_start);
    while (i < read_byte) {
        switch (read_buffer[i]) {
            case '\r':
                if ((i+1 < read_byte) && read_buffer[i+1] == '\n') {
                    i++;
                }
            case '\n':
                /* erase to line end */
                blank_coor.x = txt_coor.x;
                blank_coor.y = 0;
                blank_size.w = GUI_FONT_HEIGHT;
                blank_size.h = txt_coor.y + 1;
                gra_rect (&blank_coor, &blank_size, GUI_BG_COLOR);

                txt_coor.x += GUI_FONT_HEIGHT;
                txt_coor.y = gra_scr_h() - 1;
                /* make sure the next char coor is in this screen */
                if (txt_coor.x >= (which_scr + 1) * gra_scr_w()) {
                    i++;
                    return i;
                }
                break;
            default:
                if ((read_buffer[i] & 0x80) == 0) { /* ascii char */
                    txt_width = GUI_FONT_WIDTH;
                } else {                     /* han zi */
                    txt_width = GUI_FONT_WIDTH * 2;
                }
                if (txt_coor.y + 1 < txt_width) {
                    txt_coor.x += GUI_FONT_HEIGHT; /* next line */
                    txt_coor.y = gra_scr_h() - 1;

                    /* make sure current char coor is in this screen */
                    if (txt_coor.x >= (which_scr + 1) * gra_scr_w()) {
                        return i;
                    }
                }

                /* draw */
                han_draw_string_v (&txt_coor, READER_TEXT_COLOR, &read_buffer[i], 1);

                txt_coor.y -= txt_width; /* move cursor to right */

                if ((read_buffer[i] & 0x80) != 0) { /* han zi */
                    i++;
                }

                break;
        } /* switch (read_buffer[i]) */

        i++;

    }

    /* read byte is too little to fill the screen fully */
    _Gc.is_reach_end = 1;

    /* erase the left lines */
    /* if file not end with '\n', the last is erased also */
    blank_coor.x = txt_coor.x;
    blank_coor.y = 0;
    blank_size.w = (which_scr + 1) * gra_scr_w() - blank_coor.x;
    blank_size.h = gra_scr_h();
    gra_rect (&blank_coor, &blank_size, (5 << 11) | (5 << 5) | (5 << 0));

    return i;
}

/*==============================================================================
 * - _reader_draw_string()
 *
 * - from <p_start_coor> draw a '\0' end string
 */
static void _reader_draw_string (const GUI_COOR *p_start_coor, const uint8 *string)
{
    int txt_width;
    GUI_COOR txt_coor = *p_start_coor;
    GUI_COOR blank_coor;
    GUI_SIZE blank_size;

    while (*string) {
        if ((*string & 0x80) == 0) { /* ascii char */
            txt_width = GUI_FONT_WIDTH;
        } else {                     /* han zi */
            txt_width = GUI_FONT_WIDTH * 2;
        }

        /* check the space to right border */
        if (txt_coor.y + 1 < txt_width) { /* to right too small to draw a char */
            txt_coor.x += GUI_FONT_HEIGHT; /* next line */
            txt_coor.y = gra_scr_h() - 1;
        }

        han_draw_string_v (&txt_coor, READER_TEXT_COLOR, string, 1);

        txt_coor.y -= txt_width; /* move cursor to right */

        if ((*string & 0x80) != 0 && *(string + 1) != '\0') { /* han zi */
            string++;
        }
        string++;
    }

    /* erase to line end */
    blank_coor.x = txt_coor.x;
    blank_coor.y = 0;
    blank_size.w = GUI_FONT_HEIGHT;
    blank_size.h = txt_coor.y + 1;
    gra_rect (&blank_coor, &blank_size, GUI_BG_COLOR);
}

/*==============================================================================
 * - _reader_read_a_screen_r()
 *
 * - read from file and draw a screen from bottom to top
 */
static int _reader_read_a_screen_r (int which_scr, int read_end)
{
    int    i;
    uint8  read_buffer[READER_BUFFER_SIZE + 1];
    uint32 read_start = MAX(0, read_end - READER_BUFFER_SIZE);
    int    read_byte;
    int    txt_width;
    GUI_COOR txt_coor = {(which_scr + 1) * gra_scr_w() - GUI_FONT_HEIGHT, -1};
    /* GUI_COOR txt_coor = {(which_scr + 1) * gra_scr_w(), gra_scr_h() - 1}; */
    GUI_COOR blank_coor;
    GUI_SIZE blank_size;
    
    /* read */
    read_byte = yaffs_pread(_Gc.fd, read_buffer, read_end - read_start,
                            read_start);
    read_buffer[read_byte] = '\0';

    /* draw */
    i = read_byte - 1;
    while (i >= 0) {
        switch (read_buffer[i]) {
            case '\n':

                if (i != read_byte - 1) {
                    /* set string start y coor to leftmost */
                    txt_coor.y = gra_scr_h() - 1;

                    /* draw the string */
                    _reader_draw_string (&txt_coor, &read_buffer[i + 1]);

                    /* start a new upper line */
                    txt_coor.x -= GUI_FONT_HEIGHT;
                    txt_coor.y  = -1;
                    /* beyong the screen upper border */
                    if (txt_coor.x < which_scr * gra_scr_w()) {
                        return read_byte - i - 1;
                    }
                }

                /* terminal front string */
                read_buffer[i] = '\0';
                if ((i-1 >= 0) && read_buffer[i-1] == '\r') {
                    i--;
                    read_buffer[i] = '\0';
                }

                break;
            default:
                if ((read_buffer[i] & 0x80) == 0) { /* ascii char */
                    txt_width = GUI_FONT_WIDTH;
                } else {                     /* han zi */
                    txt_width = GUI_FONT_WIDTH * 2;
                }

                /* if there have no enough space, start a new upper line */
                if (gra_scr_h() - txt_coor.y - 1 < txt_width) {
                    txt_coor.x -= GUI_FONT_HEIGHT;
                    txt_coor.y  = -1;
                    /* beyong the screen upper border */
                    if (txt_coor.x < which_scr * gra_scr_w()) {
                        txt_coor.x += GUI_FONT_HEIGHT;
                        txt_coor.y = gra_scr_h() - 1;
                        _reader_draw_string (&txt_coor, &read_buffer[i + 1]);
                        return read_byte - i - 1;
                    }
                }
                txt_coor.y += txt_width; /* move cursor to left */

                if ((read_buffer[i] & 0x80) != 0) { /* han zi */
                    i--;
                }
                break;
        } /* switch (read_buffer[i]) */

        i--;
    }

    /* read byte is too little to fill the screen fully */
    /* in other word, we reach the file begin */

    if (txt_coor.y == -1) {
        txt_coor.x += GUI_FONT_HEIGHT;
    } else {
        /* draw file first line */
        txt_coor.y = gra_scr_h() - 1;
        _reader_draw_string (&txt_coor, &read_buffer[i + 1]);
    }

    /* erase the left lines */
    blank_coor.x = which_scr * gra_scr_w();
    blank_coor.y = 0;
    blank_size.w = txt_coor.x - blank_coor.x;
    blank_size.h = gra_scr_h();
    gra_rect (&blank_coor, &blank_size, (5 << 11) | (5 << 5) | (5 << 0));

    return read_byte - i - 1;
}

/*==============================================================================
 * - _reader_reg_context_cbi()
 *
 * - register context page cbi, only one
 */
static void _reader_reg_context_cbi ()
{
    GUI_CBI *pCBI_context;
    GUI_COOR msg_coor = {_Gc.fb_cur_x, 0};

    pCBI_context = malloc (sizeof (GUI_CBI));
    if (pCBI_context == NULL) {
        font_printf (&msg_coor, GUI_COLOR_RED, "Alloc memory failed!");
        return;
    }

    cbi_delete_all (); /* delete list | control page cbis */

    pCBI_context->left_up.x = 0;
    pCBI_context->left_up.y = 0;
    pCBI_context->right_down.x = gra_scr_w() - 1;
    pCBI_context->right_down.y = gra_scr_h() - 1;

    pCBI_context->data = NULL;

    pCBI_context->func_press   = cbf_noop;
    pCBI_context->func_leave   = cbf_noop;
    pCBI_context->func_release = _reader_cb_context_release;
    pCBI_context->func_drag    = _reader_cb_context_drag;

    cbi_register (pCBI_context);
}

/*==============================================================================
 * - _reader_cb_context_drag()
 *
 * - after drag, user release the file's context, call this
 */
static OS_STATUS _reader_cb_context_drag (GUI_CBI *pCBI_context, GUI_COOR *p_offset_coor)
{
    /* 
     * the first drag
     * we should determine user drag slide or context
     */
    if (_Gc.drag_slide_or_context == DRAG_NOOP) {
        if (GUI_ABS(p_offset_coor->y) > GUI_ABS(p_offset_coor->x)) { /* drag left <--> right */
            /* now the control icons must be hide */
            int i;
            /* copy current screen context pixels to temp array */
            for (i = 0; i < gra_scr_h(); i++) {
                memcpy (_Gc.slide_context + i * ICON_SIZE, lcd_get_addr (i, _Gc.fb_cur_x),
                        ICON_SIZE * sizeof (GUI_COLOR)); 
            }
            _Gc.drag_slide_or_context = DRAG_SLIDE;
        } else {
            _Gc.drag_slide_or_context = DRAG_CONTEXT;
        }
    }

    /* 
     * slide drag
     */
    if (_Gc.drag_slide_or_context == DRAG_SLIDE) {
        return _reader_slide_drag (NULL, p_offset_coor);
    }

    /*
     * context drag
     */
    p_offset_coor->x *= 2;

    /* maybe need to read a new screen */
    if (_Gc.fb_cur_x - p_offset_coor->x > LOW_ALARM_LEVEL + ALARM_OFFSET) {
        GUI_COOR offset = {_Gc.fb_cur_x - (LOW_ALARM_LEVEL + ALARM_OFFSET), 0};

        cbf_hw_drag (pCBI_context, &offset);
        _reader_cb_context_release (pCBI_context, NULL);

        p_offset_coor->x -= offset.x;
    }

    /* move screen */
    cbf_hw_drag (pCBI_context, p_offset_coor);

    /* update <fb_cur_x> */
    _Gc.fb_cur_x -= p_offset_coor->x;
    _Gc.fb_cur_x = MIN(MAX(0, _Gc.fb_cur_x), gra_scr_w() * 2);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_cb_context_release()
 *
 * - user release context, read a screen text maybe
 */
static OS_STATUS _reader_cb_context_release (GUI_CBI *pCBI_context, GUI_COOR *p_coor)
{
    int i;

    /* slide release */
    if (_Gc.drag_slide_or_context == DRAG_SLIDE) {
        return _reader_slide_release (NULL, NULL);
    }

    /* context release */
    if ((_Gc.fb_cur_x >= LOW_ALARM_LEVEL) && !_Gc.is_reach_end) { /* read a new down screen */
        int upper_len = (gra_scr_w() * 2) - _Gc.fb_cur_x;

        /* copy data */
        for (i = 0; i < gra_scr_h(); i++) {
            memcpy (lcd_get_addr (i, 0), lcd_get_addr (i, gra_scr_w()),
                    _Gc.fb_cur_x * sizeof (GUI_COLOR)); 
        }

        /* switch screen */
        lcd_set_show_fb_base_x (_Gc.fb_cur_x - gra_scr_w());

        /* copy data */
        for (i = 0; i < gra_scr_h(); i++) {
            memcpy (lcd_get_addr (i, _Gc.fb_cur_x),
                    lcd_get_addr (i, _Gc.fb_cur_x + gra_scr_w()),
                    upper_len * sizeof (GUI_COLOR)); 
        }

        /* change <fb_cur_x> */
        _Gc.fb_cur_x -= gra_scr_w();

        /* update pos */
        _Gc.file_start_byte += _Gc.scr0_bytes;
        _Gc.scr0_bytes = _Gc.scr1_bytes;
        _Gc.scr1_bytes = _Gc.scr2_bytes;

        /* read file to fill scr2 */
        _Gc.scr2_bytes = _reader_read_a_screen (2,
                           _Gc.file_start_byte + _Gc.scr0_bytes + _Gc.scr1_bytes);

    } else if ((_Gc.fb_cur_x <= UP_ALARM_LEVEL) && _Gc.file_start_byte) { /* read a new up screen */
        int lower_len = (gra_scr_w() * 2) -  _Gc.fb_cur_x;

        /* copy data */
        for (i = 0; i < gra_scr_h(); i++) {
            memcpy (lcd_get_addr (i, gra_scr_w() + _Gc.fb_cur_x),
                    lcd_get_addr (i, _Gc.fb_cur_x),
                    lower_len * sizeof (GUI_COLOR));
        }

        /* switch screen */
        lcd_set_show_fb_base_x (_Gc.fb_cur_x + gra_scr_w());

        /* copy data */
        for (i = 0; i < gra_scr_h(); i++) {
            memcpy (lcd_get_addr (i, gra_scr_w()), lcd_get_addr (i, 0),
                    _Gc.fb_cur_x * sizeof (GUI_COLOR)); 
        }

        /* change <fb_cur_x> */
        _Gc.fb_cur_x += gra_scr_w();

        /* update pos */
        _Gc.scr2_bytes = _Gc.scr1_bytes;
        _Gc.scr1_bytes = _Gc.scr0_bytes;

        /* read file to fill scr2 */
        _Gc.scr0_bytes = _reader_read_a_screen_r (0, _Gc.file_start_byte);

        _Gc.file_start_byte -= _Gc.scr0_bytes;

        _Gc.is_reach_end = 0;
    }

    _Gc.drag_slide_or_context = DRAG_NOOP;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_slide_to()
 *
 * - set slide last line at <pos>
 */
static void _reader_slide_to (int pos, int delay)
{
    int i = _Gc.slide_pos;

    if (i < pos) { /* make slide bigger */
        for (i++; i <= pos; i++) {
            memcpy (lcd_get_addr (i, _Gc.fb_cur_x), _Gc.slide_control + i * ICON_SIZE,
                    ICON_SIZE * sizeof (GUI_COLOR)); 
            lcd_delay(delay);
        }
    } else if (i > pos) { /* make slide smaller */
        for (; i > pos; i--) {
            memcpy (lcd_get_addr (i, _Gc.fb_cur_x), _Gc.slide_context + i * ICON_SIZE,
                    ICON_SIZE * sizeof (GUI_COLOR)); 
            lcd_delay(delay);
        }
    }

    _Gc.slide_pos = pos;
}

/*==============================================================================
 * - _reader_slide_drag()
 *
 * - drag slide left <--> right
 */
static OS_STATUS _reader_slide_drag (GUI_CBI *pCBI_slide, GUI_COOR *p_offset_coor)
{
    int end_pos = _Gc.slide_pos;

    if (p_offset_coor->y > 0) { /* drag to left */

        end_pos = MIN(gra_scr_h() - 1, _Gc.slide_pos + p_offset_coor->y);
    } else if (p_offset_coor->y < 0) { /* drag to right */

        end_pos = MAX(-1, _Gc.slide_pos + p_offset_coor->y);
    }
    _reader_slide_to (end_pos, 0);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_slide_release()
 *
 * - determine go to control page or stay context page
 */
static OS_STATUS _reader_slide_release (GUI_CBI *pCBI_slide, GUI_COOR *coor)
{
    int end_pos;

    if (_Gc.slide_pos >= (gra_scr_h() / 4)) {
        end_pos = gra_scr_h() - 1;
        _reader_slide_to (end_pos, SLIDE_DELAY);
        _reader_reg_control_cbis ();
    } else {
        end_pos = -1;
        _reader_slide_to (end_pos, SLIDE_DELAY);
    }

    _Gc.drag_slide_or_context = DRAG_NOOP;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_cb_ctrl_hide()
 *
 * - hide control bar and back to context page
 */
static OS_STATUS _reader_cb_ctrl_hide (GUI_CBI *pCBI_hide, GUI_COOR *coor)
{
    /* hide the control bar */
    _reader_slide_to (-1, SLIDE_DELAY);

    /* register context cbi and switch to context page */
    _reader_reg_context_cbi ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_cb_ctrl_auto()
 *
 * - auto scroll down
 */
static OS_STATUS _reader_cb_ctrl_auto (GUI_CBI *pCBI_auto, GUI_COOR *coor)
{
    /* hide the control bar */
    _reader_slide_to (-1, 0);

    /* register context cbi and switch to context page */
    _reader_reg_context_cbi ();

    /* set to auto mode */
    _Gc.auto_mode = 1;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_cb_ctrl_begin()
 *
 * - the control page begin cbi's callback, this operation
 *   quit control page and back to context page, and go to file begin
 */
static OS_STATUS _reader_cb_ctrl_begin (GUI_CBI *pCBI_begin, GUI_COOR *coor)
{
    /* init position to 0 */
    _Gc.file_start_byte = 0;
    _Gc.fb_cur_x        = 0;

    _Gc.is_reach_end = 0;

    _Gc.slide_pos = -1;

    /* read and draw file text */
    _Gc.scr0_bytes =  _reader_read_a_screen (0, _Gc.file_start_byte);
    _Gc.scr1_bytes =  _reader_read_a_screen (1, _Gc.file_start_byte + _Gc.scr0_bytes);
    _Gc.scr2_bytes =  _reader_read_a_screen (2,
                               _Gc.file_start_byte + _Gc.scr0_bytes + _Gc.scr1_bytes);

    /* show first fb */
    gra_set_show_fb (0);

    /* register context cbi and switch to context page */
    _reader_reg_context_cbi ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_cb_ctrl_end()
 *
 * - the control page end cbi's callback, this operation
 *   quit control page and back to context page, and go to file end
 */
static OS_STATUS _reader_cb_ctrl_end (GUI_CBI *pCBI_end, GUI_COOR *coor)
{
    /* init position to end */
    _Gc.file_start_byte = yaffs_lseek (_Gc.fd, 0, SEEK_END);
    _Gc.fb_cur_x        = gra_scr_w() * 2;

    _Gc.is_reach_end = 1;

    _Gc.slide_pos = -1;

    /* read and draw file text */
    _Gc.scr2_bytes =  _reader_read_a_screen_r (2, _Gc.file_start_byte);
    _Gc.file_start_byte -= _Gc.scr2_bytes;
    _Gc.scr1_bytes =  _reader_read_a_screen_r (1, _Gc.file_start_byte);
    _Gc.file_start_byte -= _Gc.scr1_bytes;
    _Gc.scr0_bytes =  _reader_read_a_screen_r (0, _Gc.file_start_byte);
    _Gc.file_start_byte -= _Gc.scr0_bytes;

    /* show last fb */
    gra_set_show_fb (2);

    /* register context cbi and switch to context page */
    _reader_reg_context_cbi ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_cb_ctrl_list()
 *
 * - back to file list. quit control page and back to list page
 */
static OS_STATUS _reader_cb_ctrl_list (GUI_CBI *pCBI_list, GUI_COOR *coor)
{
    TXT_FILE_NODE *p_file_node = NULL;

    /* free file node memory */
    p_file_node = (TXT_FILE_NODE *)dlist_get (&_G_file_list);
    while (p_file_node != NULL) {
        free (p_file_node);
        p_file_node = (TXT_FILE_NODE *)dlist_get (&_G_file_list);
    }

    /* close file */
    yaffs_close(_Gc.fd);

    /* save file position */
    _G_files_pos[_Gc.file_pos_index].start_byte = _Gc.file_start_byte;
    _G_files_pos[_Gc.file_pos_index].fb_cur_x = _Gc.fb_cur_x;

    /* free slide context control icon memory */
    free (_Gc.slide_context);
    free (_Gc.slide_control);

    /* restart reader application with don't read position file */
    app_reader (NULL, NULL);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _reader_dump_control_icons()
 *
 * - dump control page cbi's icon to memory <_Gc.slide_control>
 */
static void _reader_dump_control_icons ()
{
    int i, j;
    GUI_COLOR *icon_pixels;
    int icon_width;
    int icon_height;
    int icon_start_y;
    char *icon_file_name[CONTROL_CBI_NUM] = {
                                "/n1/gui/cbi_fast_down_t.jpg",
                                "/n1/gui/cbi_fast_up_t.jpg",
                                "/n1/gui/cbi_list_t.jpg",
                                "/n1/gui/cbi_auto_t.jpg",
                                "/n1/gui/cbi_up.jpg"
    };

    _Gc.slide_context = malloc (ICON_SIZE * gra_scr_h() * sizeof (GUI_COLOR));
    _Gc.slide_control = calloc (1, ICON_SIZE * gra_scr_h() * sizeof (GUI_COLOR));

    for (i = 0; i < CONTROL_CBI_NUM; i++) {
        icon_start_y = i * ICON_SIZE;
        icon_pixels = get_pic_pixel (icon_file_name[i], &icon_width, &icon_height);

        for (j = 0; j < ICON_SIZE; j++) {
            memcpy (_Gc.slide_control + (icon_start_y + j) * ICON_SIZE,
                    icon_pixels + j * ICON_SIZE,
                    ICON_SIZE * sizeof (GUI_COLOR)); 
        }
        free (icon_pixels);
    }
}

/*==============================================================================
 * - _reader_reg_control_cbis()
 *
 * - delete context cbi and register control cbis
 */
static void _reader_reg_control_cbis ()
{
    int i;
    GUI_CBI *pCBI_ctrl;
    GUI_COOR left_up;
    GUI_COOR right_down;
    CB_FUNC_PTR funcs[CONTROL_CBI_NUM] = {
                _reader_cb_ctrl_end,
                _reader_cb_ctrl_begin,
                _reader_cb_ctrl_list,
                _reader_cb_ctrl_auto,
                _reader_cb_ctrl_hide
    };

    cbi_delete_all (); /* delete context cbi */

    for (i = 0; i < CONTROL_CBI_NUM; i++) {
        left_up.x = 0;
        left_up.y = i * ICON_SIZE;
        right_down.x = left_up.x + ICON_SIZE - 1;
        right_down.y = left_up.y + ICON_SIZE - 1;

        pCBI_ctrl = cbi_create_blank (&left_up, &right_down, cbf_default_press);
        pCBI_ctrl->func_release = funcs[i];
    }
}

/*==============================================================================
** FILE END
==============================================================================*/
