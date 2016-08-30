/*==============================================================================
** telnet.c -- shan zai telnet server.
**
** MODIFY HISTORY:
**
** 2011-08-30 wdf Create.
** 2011-10-13 wdf change telnet_stor() return value.
==============================================================================*/

#include <dfewos.h>
#include "../../string.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"

/*======================================================================
  telnet server used port(UDP)
======================================================================*/
#define TELNET_PORT             23

/*======================================================================
  global variables
======================================================================*/
#define TELNET_BUFFER_SIZE     (10 * KB)
static int  _G_telnet_open = 0;             /* is telnet server is open */
static int  _G_telnet_buf_context = 0;      /* current buffer size */
static char _G_telnet_buffer[TELNET_BUFFER_SIZE]; /* buffer for send to client */

/*==============================================================================
 * - telnet_stor()
 *
 * - shor a char in _G_telnet_buffer. called by serial_putc()
 */
int telnet_stor (const char c)
{
    static int in_vt100_ctrl = 0;

    /* check is there need to stor char */
    if (_G_telnet_open == 0) {
        return 0;
    }

    if (c == '\033') {            /* filter vt100 ctrl char */
        in_vt100_ctrl = 1;
        return 1;
    }
    if (in_vt100_ctrl) {
        if (c == 'm') {
            in_vt100_ctrl = 0;
        }
        return 1;
    }                             /* filter vt100 ctrl char */

    /* stor char */
    if (_G_telnet_buf_context < TELNET_BUFFER_SIZE) {
        _G_telnet_buffer[_G_telnet_buf_context++] = c;
    }

    return 1;
}

/*==============================================================================
 * - T_telnet_server_start()
 *
 * - fake telnet server task
 */
void T_telnet_server_start ()
{
    int                 socket_id;
    struct sockaddr_in  serv_addr;
    struct sockaddr_in  clnt_addr;
    char                recv_buff[256];
    int                 recv_len;
    socklen_t           addr_len = sizeof(struct sockaddr_in);
    int                 lwip_ret;
    char               *start_cmd;

    socket_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_id < 0) {
        serial_printf("telnet: socket() failed!\n");
        return;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_len    = sizeof(struct sockaddr_in);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(TELNET_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    lwip_ret = bind(socket_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (lwip_ret < 0) {
        serial_printf("telnet: bind() failed!\n");
        goto over;
    }

    FOREVER {
        recv_len = recvfrom (socket_id, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&clnt_addr, &addr_len);
        if (recv_len <= 0) {
            if (errno != ETIMEDOUT && errno != EWOULDBLOCK) {
                serial_printf("telnet: recvfrom () failed!\n");
                goto over;
            }
            continue;
        }

        _G_telnet_open = 1;       /* when serial_putc() stor in _G_telnet_buffer */
        _G_telnet_buf_context = 0;                  /* clear output buffer */

        /*
         * delete user input head space char
         */
        recv_buff[recv_len] = '\0';                 /* terminal user data */
        start_cmd = recv_buff;
        while (isspace(*start_cmd)) {
        	start_cmd++;
        }

        if (strlen(start_cmd) == 0) { /* cmd_line have no char except blank */
            serial_puts("-> ");
        } else {                      /* deal with cmd_line */

            if ((strncmp(start_cmd, "vi", 2)) == 0 && 
                    ((strlen(start_cmd) == 2) || (isspace(start_cmd[2])))) {           /* is "vi" command */
                serial_puts("\nDon't use VI!");
            } else {
                cmd_do(start_cmd);
            }
            serial_puts("\n-> ");
        }

        sendto (socket_id, _G_telnet_buffer, _G_telnet_buf_context, 0, (const struct sockaddr *)&clnt_addr, addr_len);
        _G_telnet_open = 0;       /* when serial_putc() not stor int _G_telnet_buffer */
    }

over:
    _G_telnet_open = 0;
    close(socket_id);
}

/*==============================================================================
** FILE END
==============================================================================*/

