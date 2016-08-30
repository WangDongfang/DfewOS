/*==============================================================================
** msgQ.h -- the messgage queue module header.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __MSGQ_H__
#define __MSGQ_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  the msgQ struct
======================================================================*/
typedef struct msg_queue {
    DL_NODE  msgQ_list_node; /* for add to _G_msgQ_list */

    uint32   max_num;
    uint32   max_len;
    uint32   cur_num;
    DL_LIST  msg_list;
    DL_LIST  wait_send_list;
    DL_LIST  wait_recv_list;
} MSG_QUE;

/*======================================================================
  msgQ module support routines for other modules
======================================================================*/
MSG_QUE *msgQ_init (MSG_QUE *pNewMsgQ,
                    uint32   max_num, 
                    uint32   max_len);

OS_STATUS msgQ_send (MSG_QUE     *pMsgQ, 
                     const void *buffer,
                     uint32       buf_len,
                     uint32       timeout);

OS_STATUS msgQ_receive (MSG_QUE *pMsgQ,
                        void   *buffer,
                        uint32   buf_len,
                        uint32   timeout);
OS_STATUS msgQ_delete (MSG_QUE *pMsgQ);
OS_STATUS msgQ_remove (MSG_QUE *pMsgQ, OS_TCB *pTcb);
void      msgQ_decrease ();
void      msgQ_show (EACH_FUNC_PTR show_routine);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MSGQ_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

