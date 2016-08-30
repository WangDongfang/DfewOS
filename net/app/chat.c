/*==============================================================================
** chat.c -- remote check the board is alive of not.
**
** MODIFY HISTORY:
**
** 2011-09-02 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include "../../string.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"

/*======================================================================
  char server used port(UDP)
======================================================================*/
#define CHAT_PORT          9888

/*======================================================================
  char server config
======================================================================*/
#define RECV_BUF_SIZE      256
#define SEND_BUF_SIZE      RECV_BUF_SIZE + 128
#define ADD_MSG            "I am still alive. You just said: - "
#define SMILE              " - ^_^"

/*==============================================================================
 * - T_chat_server_start()
 *
 * - chat server task
 */
void T_chat_server_start ()
{
    int                 socket_id;
    struct sockaddr_in  serv_addr;
    struct sockaddr_in  clnt_addr;
    char                recv_buff[RECV_BUF_SIZE];
    char                send_buff[SEND_BUF_SIZE];
    int                 recv_len;
    socklen_t           addr_len = sizeof(struct sockaddr_in);
    int                 lwip_ret;
    int                 dirty_times = 0;

    socket_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_id < 0) {
        serial_printf("chats: socket() failed!\n");
        return;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_len  = sizeof(struct sockaddr_in);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(CHAT_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    lwip_ret = bind(socket_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (lwip_ret < 0) {
        serial_printf("chats: bind() failed!\n");
        goto fail;
    }

    FOREVER {
        recv_len = recvfrom(socket_id, recv_buff, RECV_BUF_SIZE - 1, 0, (struct sockaddr *)&clnt_addr, &addr_len);
        if (recv_len <= 0) {
            if (errno != ETIMEDOUT && errno != EWOULDBLOCK) {
                serial_printf("chats: recvfrom () failed!\n");
                goto fail;
            }
            continue;
        }

        recv_buff[recv_len] = '\0';
        strcpy (send_buff, ADD_MSG);
        if ((!strcmp (recv_buff, "fuck")) ||
            (!strcmp (recv_buff, "shit"))) {
            dirty_times++;
            if (dirty_times < 3) {
                strcat (send_buff, "不要说脏话");
            } else {
                strcat (send_buff, "悟空，削他");
            }
        } else if (!strcmp (recv_buff, "fuck you")) {
            strcat (send_buff, "YOU FUCK YOUSELF!");
        } else {
            strcat (send_buff, recv_buff);
        }
        strcat (send_buff, SMILE);

        sendto(socket_id, send_buff, (strlen(send_buff) + 1), 0, (const struct sockaddr *)&clnt_addr, addr_len);
    }

fail:
    close(socket_id);
}

/*==============================================================================
** FILE END
==============================================================================*/

