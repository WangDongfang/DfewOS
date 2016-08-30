/*==============================================================================
** gui_core.c -- gui core, do message.
**
** MODIFY HISTORY:
**
** 2011-10-28 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "gui.h"

/*======================================================================
  Global variables
======================================================================*/
static int      _G_msg_lost = 0;
static MSG_QUE  _G_msg_queue;

/*======================================================================
  forward function declare
======================================================================*/
static void _T_gui_job_thread ();

/*==============================================================================
 * - gui_core_init()
 *
 * - gui core init. create message queue and start the message job task
 */
OS_STATUS gui_core_init ()
{   
    /* create message queue */
    if (msgQ_init(&_G_msg_queue, GUI_MSG_QUEUE_NUM_MAX, sizeof(GUI_MSG)) == NULL) {
        return OS_STATUS_ERROR;
    }

    /* start task 'tGui' */
    /* This task pend on message queue, if get a message,
     * call cbi' callback function */
    if (task_create("tGui", GUI_TASK_STACK_SIZE, GUI_TASK_PRIORITY,
                        _T_gui_job_thread, 0, 0) == NULL) {
    	msgQ_delete(&_G_msg_queue);

    	return OS_STATUS_ERROR;
    } else {
    	return OS_STATUS_OK;
    }
}

/*==============================================================================
 * - gui_job_add()
 *
 * - touch screen driver call this to post message
 */
OS_STATUS gui_job_add (GUI_COOR *pCoor, GUI_MSG_TYPE type)
{
    GUI_MSG msg = {type, *pCoor};

    if (msgQ_send(&_G_msg_queue, &msg, sizeof(msg), NO_WAIT)
            == OS_STATUS_ERROR) {
        serial_printf("gui lost message!\n");
        _G_msg_lost++;
        return OS_STATUS_ERROR;
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _T_gui_job_thread()
 *
 * - the process thread
 */
static void _T_gui_job_thread ()
{
    GUI_MSG  msg;                   /* store read message */
    GUI_CBI *pCBI_pre = NULL;       /* last operat cbi */
    GUI_COOR pre_coor;              /* last pressed coordinate */
    GUI_CBI *pCBI_cur = NULL;       /* current operat cbi */
    GUI_COOR cbi_coor;              /* current pressed coordinate in cbi */
    GUI_COOR offset;                /* current - last screen coordinate */

    FOREVER {
        msgQ_receive(&_G_msg_queue, &msg, sizeof(msg), WAIT_FOREVER);

        /* find press which CBI */
        pCBI_cur = cbi_in_which (&msg.scr_coor, &cbi_coor); /* if find, change screen coordinate
                                            to cbi inner coordinate */

        switch (msg.type) {
            case GUI_MSG_TOUCH_DOWN:
                if (pCBI_cur != pCBI_pre) { /* 这次和上次按在不同的 CBI 上 */
                    if (pCBI_cur == NULL) { /* we go to the wild */
                        offset.x = msg.scr_coor.x - pre_coor.x;  /* move interval */
                        offset.y = msg.scr_coor.y - pre_coor.y;
                        pCBI_pre->func_drag (pCBI_pre, &offset);
                    } else {                /* we go to other cbi */
                        /* leave previous CBI */
                        if (pCBI_pre != NULL) {
                            pCBI_pre->func_leave (pCBI_pre, &msg.scr_coor);
                        }

                        /* store current CBI to pre */
                        pCBI_pre = pCBI_cur;

                        /* press current CBI */
                        if (pCBI_cur->func_press != cbf_default_press &&
                            pCBI_cur->func_press != cbf_noop) {

                            cbf_default_press (pCBI_cur, &cbi_coor);
                        }
                        pCBI_cur->func_press (pCBI_cur, &cbi_coor);
                    }

                } else { /* (pCBI_cur == pCBI_pre)  连着在一个 CBI 上按两次 */
                    if (pCBI_cur != NULL) {
                        offset.x = msg.scr_coor.x - pre_coor.x;  /* move interval */
                        offset.y = msg.scr_coor.y - pre_coor.y;
                        pCBI_cur->func_drag (pCBI_cur, &offset);
                    }
                }

                /* store current screen coordinate to pre */
                pre_coor = msg.scr_coor;

                break;
            case GUI_MSG_TOUCH_UP:
                if (pCBI_cur != NULL) {
                    pCBI_cur->func_release (pCBI_cur, &cbi_coor);
                } else if (pCBI_pre != NULL) { /* cur is null & pre is not null */
                    cbi_coor.x = msg.scr_coor.x - pCBI_pre->left_up.x;
                    cbi_coor.y = msg.scr_coor.y - pCBI_pre->left_up.y;
                    pCBI_pre->func_release (pCBI_pre, &cbi_coor);
                }
                pCBI_pre = NULL;
                break;
            default:
                break;
        }

    }

}

/*==============================================================================
** FILE END
==============================================================================*/

