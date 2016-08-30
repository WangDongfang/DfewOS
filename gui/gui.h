/*==============================================================================
** gui_core.h -- gui header.
**
** MODIFY HISTORY:
**
** 2011-10-28 wdf Create.
==============================================================================*/

#ifndef __GUI_H__ 
#define __GUI_H__

/*======================================================================
  configs
======================================================================*/
#define GUI_CBI_NAME_LEN_MAX      100
#define GUI_TASK_STACK_SIZE      (80 * KB)
#define GUI_TASK_PRIORITY         8
#define GUI_MSG_QUEUE_NUM_MAX     50

/*======================================================================
  color configs
======================================================================*/
#define  GUI_COLOR              uint16
#define  GUI_COLOR_BLACK        0x0000
#define  GUI_COLOR_WHITE        0xFFFF
#define  GUI_COLOR_RED          0xF800
#define  GUI_COLOR_GREEN        0x07E0
#define  GUI_COLOR_BLUE         0x001F
#define  GUI_COLOR_YELLOW      (GUI_COLOR_RED | GUI_COLOR_GREEN)
#define  GUI_COLOR_CYAN        (GUI_COLOR_GREEN | GUI_COLOR_BLUE)
#define  GUI_COLOR_MAGENTA     (GUI_COLOR_RED | GUI_COLOR_BLUE)

#define  GUI_BG_COLOR           GUI_COLOR_BLACK

/*======================================================================
  font information
======================================================================*/
#define  GUI_FONT_WIDTH         8 
#define  GUI_FONT_HEIGHT        16

/*======================================================================
  utils
======================================================================*/
#define GUI_ABS(x)     (((x) < 0) ? (-(x)) : (x))
#define GUI_ASSERT(b)  if (!(b)) {serial_printf("GUI ASSERT");FOREVER{}}


/*======================================================================
  这是 GUI 用到的坐标结构体
======================================================================*/
typedef struct gui_coordinate {
    int x;
    int y;
} GUI_COOR;

/*======================================================================
  这是 GUI 用到的矩形大小结构体
======================================================================*/
typedef struct gui_size {
    int w;
    int h;
} GUI_SIZE;

/*======================================================================
  这是触摸屏驱动向 GUI 消息队列中发送的消息类型
======================================================================*/
typedef enum gui_message_type {
    GUI_MSG_TOUCH_DOWN,
    GUI_MSG_TOUCH_UP 
} GUI_MSG_TYPE;

/*======================================================================
  这是 GUI 消息队列中的消息结构体
======================================================================*/
typedef struct gui_message {
    GUI_MSG_TYPE type;
    GUI_COOR     scr_coor;
} GUI_MSG;

/*======================================================================
  这是 CBI 回调函数的类型
======================================================================*/
typedef struct callback_item  GUI_CBI;
typedef OS_STATUS (*CB_FUNC_PTR) (GUI_CBI *, GUI_COOR *);

/*======================================================================
  这是 CBI (callback item) 是出现在屏幕上可接受消息的方块
======================================================================*/
struct callback_item {

    DL_NODE  cbi_list_node;

    /* name */
    char name[GUI_CBI_NAME_LEN_MAX];

    /* covered area */
    GUI_COOR left_up;
    GUI_COOR right_down;

    /* message operat functions */
    CB_FUNC_PTR func_press;
    CB_FUNC_PTR func_leave;
    CB_FUNC_PTR func_release;
    CB_FUNC_PTR func_drag;

    GUI_COLOR *data;
};

#define ICON_SIZE               96
typedef struct icon_dbi {
    char name[GUI_CBI_NAME_LEN_MAX];
    CB_FUNC_PTR func;
} ICON_CBI;

/*======================================================================
  APIs
======================================================================*/
/* gui_core.c */
OS_STATUS gui_job_add (GUI_COOR *pCoor, GUI_MSG_TYPE type);
OS_STATUS gui_core_init ();

/* gui_cbi.c */
GUI_CBI  *cbi_in_which (const GUI_COOR *p_scr_coor, GUI_COOR *p_cbi_coor);
OS_STATUS cbi_init (GUI_CBI *pCBI);
GUI_CBI  *cbi_create_default (const char     *pic_file_name,
                              const GUI_COOR *p_left_up,
                              GUI_SIZE       *p_cbi_size,
                              BOOL            save_data);
GUI_CBI  *cbi_create_blank (GUI_COOR *p_left_up, GUI_COOR *p_right_bottom,
                            CB_FUNC_PTR user_func_press);
OS_STATUS cbi_register (GUI_CBI *pCBI);
OS_STATUS cbi_unregister (GUI_CBI *pCBI);
OS_STATUS cbi_delete_all ();
OS_STATUS cbi_cover_all ();
OS_STATUS cbi_uncover ();

/* gui_cbf.c */
OS_STATUS cbf_default_press (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);
OS_STATUS cbf_default_leave (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);
OS_STATUS cbf_default_release (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);
OS_STATUS cbf_no_drag (GUI_CBI *pCBI, GUI_COOR *p_offset_coor);
OS_STATUS cbf_do_drag (GUI_CBI *pCBI, GUI_COOR *p_offset_coor);
OS_STATUS cbf_hw_drag (GUI_CBI *pCBI, GUI_COOR *p_offset_coor);
OS_STATUS cbf_noop (GUI_CBI *pCBI, GUI_COOR *p_coor);
OS_STATUS cbf_go_home (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);

/* gui_gra.c */
GUI_COLOR *gra_get_block (const GUI_COOR *p_start, const GUI_SIZE *p_size);
void gra_block (const GUI_COOR *p_start, const GUI_SIZE *p_size, const GUI_COLOR *p_data);
void gra_block_t (const GUI_COOR *p_start, const GUI_SIZE *p_size, const GUI_COLOR *p_data);
void gra_rect (const GUI_COOR *p_start, const GUI_SIZE *p_size, GUI_COLOR color);
OS_STATUS gra_line (const GUI_COOR *p_start, const GUI_COOR *p_end, GUI_COLOR color, int width);
void gra_line_u (const GUI_COOR *p_start, const GUI_COOR *p_end, GUI_COLOR color, int width);
void gra_clear (GUI_COLOR color);
void gra_get_scr_size (GUI_SIZE *p_size);
int  gra_scr_w ();
int  gra_scr_h ();
void *gra_set_show_fb (int index);
void gra_set_pixel (const GUI_COOR *p_coor, GUI_COLOR color);
GUI_COLOR gra_get_pixel (const GUI_COOR *p_coor);

/* gui_font.c */
void font_draw_string (const GUI_COOR *s, GUI_COLOR color, const uint8 *string, int count);
void font_draw_string_t (const GUI_COOR *s, GUI_COLOR color, const uint8 *string, int count);
void font_draw_string_v (const GUI_COOR *s, GUI_COLOR color, const uint8 *string, int count);
void font_printf (const GUI_COOR *start, GUI_COLOR color, const char *fmt, ...);

/* gui_han.c */
void han_init ();
void han_draw_char (const GUI_COOR *s, uint16 color, const uint8 *string, int have_bg);
void han_draw_char_v (const GUI_COOR *s, uint16 color, const uint8 *string);
void han_draw_string (const GUI_COOR *s, uint16 color, const uint8 *string, int count);
void han_draw_string_t (const GUI_COOR *s, uint16 color, const uint8 *string, int count);
void han_draw_string_v (const GUI_COOR *s, uint16 color, const uint8 *string, int count);
#endif /* __GUI_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

