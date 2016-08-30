/*==============================================================================
** gui_tet_net.c -- tetris game receive message from PC task.
**
** MODIFY HISTORY:
**
** 2011-11-25 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../../string.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "../gui.h"

extern uint16 *lcd_get_addr (int row, int col);
extern int  bri_send_msg (int type);

/*======================================================================
  debug macro
======================================================================*/
#define TET_NET_DEBUG(msg)  {\
    GUI_COOR recv_msg_coor = {ICON_SIZE, 10};\
    font_printf (&recv_msg_coor, GUI_COLOR_RED, msg);\
}


/*======================================================================
  tetris recv task used port(UDP)
======================================================================*/
#define TETRIS_RECV_PORT     9889
#define PC_SERVER_IP		 "192.168.1.238"
#define TETRIS_SEND_PORT     9890

/*======================================================================
  Tet_Net Control. One tetris net have a instance of this struct
======================================================================*/
typedef struct Tet_Net_Ctrl {
    int                 send_socket_id;
    struct sockaddr_in  send_addr;
} TET_NET_CTRL;

/*======================================================================
  forward functions declare
======================================================================*/
static void _tet_net_send_scr ();
static void _tet_net_send_pic (uint16 **p_blocks, int block_num,
                               int block_w, int block_h);
static void _send_int (int value);
static void _send_mem (void *p, int size);
static void _T_tet_net_recv ();

/*======================================================================
  Global variables
======================================================================*/
static TET_NET_CTRL *_G_Tet_Net_ID = NULL;

/*==============================================================================
 * - tet_net_init()
 *
 * - init net connection socket and start recv task
 */
void tet_net_init (uint16 **p_blocks, int block_num, int block_w, int block_h)
{
    if (_G_Tet_Net_ID != NULL) {
        return ;
    }

    _G_Tet_Net_ID = (TET_NET_CTRL *)malloc (sizeof(TET_NET_CTRL));

    if (_G_Tet_Net_ID == NULL) {
        TET_NET_DEBUG ("There is no memory for <Bridge_ID>!");
        return;
    }

    /* create send socket */
    _G_Tet_Net_ID->send_socket_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    /* init send address */
    memset(&_G_Tet_Net_ID->send_addr, 0, sizeof(struct sockaddr_in));
    _G_Tet_Net_ID->send_addr.sin_len    = sizeof(struct sockaddr_in);
    _G_Tet_Net_ID->send_addr.sin_family = AF_INET;
    _G_Tet_Net_ID->send_addr.sin_port   = htons(TETRIS_SEND_PORT);
    inet_aton(PC_SERVER_IP, &(_G_Tet_Net_ID->send_addr.sin_addr));

    /* send whole screen */
    _tet_net_send_scr ();

    /* send all blocks' pixels */
    _tet_net_send_pic (p_blocks, block_num, block_w, block_h);


    task_create("tTet_R", 10240, 20, _T_tet_net_recv, 0, 0);
}

/*==============================================================================
 * - tet_net_send_show()
 *
 * - send show message to PC
 */
int tet_net_send_show (int x, int y, int show)
{
    if (_G_Tet_Net_ID == NULL) {
        return -1;
    }

    _send_int (x | 0x80000000);
    _send_int (y | 0x40000000);
    _send_int (show | 0x20000000);

    return 0;
}

/*==============================================================================
 * - tet_net_free()
 *
 * - quit net connection
 */
void tet_net_free ()
{
    if (_G_Tet_Net_ID == NULL) {
        return;
    }

    close (_G_Tet_Net_ID->send_socket_id);

    if (_G_Tet_Net_ID != NULL) {
        free (_G_Tet_Net_ID);
    }
    _G_Tet_Net_ID = NULL;
}

/*==============================================================================
 * - _tet_net_send_scr()
 *
 * - send screen picture to PC
 */
static void _tet_net_send_scr ()
{
    int i = 0;
    void *p = NULL;
    int size = gra_scr_w() * sizeof(GUI_COLOR);

    _send_int (gra_scr_w()); /* send screen width */
    _send_int (gra_scr_h()); /* send screen height */

    /* send screen pixels */
    for (; i < gra_scr_h(); i++) {
        p = lcd_get_addr (i, 0);

        _send_mem (p, size);
    }
}

/*==============================================================================
 * - _tet_net_send_pic()
 *
 * - send block pictures to PC
 */
static void _tet_net_send_pic (uint16 **p_blocks, int block_num, int block_w, int block_h)
{
	int i;
    _send_int (block_num);
    _send_int (block_w);
    _send_int (block_h);
    for (i = 1; i <= block_num; i++) {
        _send_mem (p_blocks[i], block_w * block_h * sizeof(uint16));
    }
}

/*==============================================================================
 * - _send_int()
 *
 * - send a integer to PC
 */
static void _send_int (int value)
{
    socklen_t addr_len = sizeof(struct sockaddr_in);

    sendto(_G_Tet_Net_ID->send_socket_id, &value, sizeof(int), 0,
           (const struct sockaddr *)&_G_Tet_Net_ID->send_addr, addr_len);
}

/*==============================================================================
 * - _send_mem()
 *
 * - send a block of memory to PC
 */
static void _send_mem (void *p, int size)
{
    socklen_t addr_len = sizeof(struct sockaddr_in);

    sendto(_G_Tet_Net_ID->send_socket_id, p, size, 0,
           (const struct sockaddr *)&_G_Tet_Net_ID->send_addr, addr_len);
}

/*==============================================================================
 * - _T_tet_net_recv()
 *
 * - tetris game receive message from PC task
 */
static void _T_tet_net_recv ()
{
    int                 socket_id;
    struct sockaddr_in  serv_addr;
    struct sockaddr_in  clnt_addr;
    int                 msg;
    int                 recv_len;
    socklen_t           addr_len = sizeof(struct sockaddr_in);
    int                 lwip_ret;

    GUI_COOR            home_coor = {10, ICON_SIZE * 2 + 10};
    GUI_COOR            recv_msg_coor = {ICON_SIZE, 10};

    socket_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_id < 0) {
        TET_NET_DEBUG("tet_net: socket() failed!");
        return;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_len  = sizeof(struct sockaddr_in);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(TETRIS_RECV_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    lwip_ret = bind(socket_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (lwip_ret < 0) {
        TET_NET_DEBUG("tet_net: bind() failed!");
        goto fail;
    }

    FOREVER {
        if (_G_Tet_Net_ID == NULL) {
            TET_NET_DEBUG("tet_net: exit");
            break;
        }
        recv_len = recvfrom(socket_id, &msg, sizeof(int), 0, (struct sockaddr *)&clnt_addr, &addr_len);
        if (recv_len <= 0) {
            if (errno != ETIMEDOUT && errno != EWOULDBLOCK) {
                TET_NET_DEBUG("tet_net: recvfrom () failed!");
                goto fail;
            }
            continue;
        }
        font_printf (&recv_msg_coor, GUI_COLOR_RED, "Recv: %d", msg);
        
        if (msg == 0) {
            gui_job_add (&home_coor, GUI_MSG_TOUCH_DOWN);
            gui_job_add (&home_coor, GUI_MSG_TOUCH_UP);
            break;
        } else {
            bri_send_msg (msg);
        }
    }

fail:
    close(socket_id);
}

/*==============================================================================
** FILE END
==============================================================================*/

