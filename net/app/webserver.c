/*==============================================================================
** webserver.c -- a simple http server.
**
** MODIFY HISTORY:
**
** 2011-09-07 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../../string.h"
#include "yaffs_guts.h"
#include "lwip/netdb.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "ftp_code.h"
                                  
#define PORT          80                      // The port which is communicate with server
#define BACKLOG       5
#define LENGTH        1024                    // Buffer length    
#define PATH_LEN_MAX  256
#define WEB_PATH     "/n1/"
#define HOME_HTML    "home.html"

#define WEB_LOG(fmt, ...)

char  reply[]={
    "HTTP/1.0 200 OK\r\n"
    "Date: Thu, 08 Sep 2011 10:26:00 GMT\r\n"
    "Server: Dfew CO.,LTD\r\n"
    "Accept-Ranges: bytes\r\n"
    "Connection: Keep-Close\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"};
char  html[]={
#if 1
    "<HTML>\r\n"
    "<HEAD>\r\n"
    "<TITLE>DFEWOS -- LWIP 演示网页</TITLE>\r\n"
    "<BODY aLink=green bgColor=#f1f1dd link=red\r\n"
    "vLink=#321afd>\r\n"
    "<H1>Welcome To Dfewos Demo</H1>\r\n"
    "<UL>\r\n"
    "<LI> <A HREF=home.html>DFEWOS home</A>\r\n"
    "</BODY>\r\n"
    "</HTML>\r\n"
#endif
            };

char html_start[] = {
    "<HTML>\r\n"
    "<HEAD>\r\n"
    "<TITLE>DFEWOS -- 文本文件 </TITLE>\r\n"
    "<BODY aLink=green bgColor=#f1f1dd link=red vLink=#321afd>\r\n"
    "<H1>"
};
char html_end[] = {
    "</H1>\r\n"
    "<UL>\r\n"
    "<LI> <A HREF=\"#top\">return top</A>\r\n"
    "<BR><BR><LI> <A HREF=http://192.168.1.167/home.html>HOME PAGE</A>\r\n"
    "</BODY>\r\n"
    "</HTML>\r\n"
};

char  httpgif[]={
    "HTTP/1.0 200 OK\r\n"
    "Date: Mon, 24 Nov 2005 10:26:00 GMT\r\n"
    "Server: microHttp/1.0 Dfew CO.,LTD\r\n"
    "Accept-Ranges: bytes\r\n"
    "Connection: Keep-Close\r\n"
    "Content-Type: image/bmp\r\n"
    "\r\n"};
char bmp[] = {
    "BM"
};
 
                                                                             
/*==============================================================================
 * - T_web_server_start()
 *
 * - http server task
 */
void T_web_server_start ()
{   
    int sockfd;                        // Socket file descriptor
    int nsockfd;                       // New Socket file descriptor
    int num;
    socklen_t       sin_size;                      // to store struct size
    char revbuf[LENGTH]; 
    struct sockaddr_in addr_local;     
    struct sockaddr_in addr_remote;    
               
    /* Get the Socket file descriptor */  
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )  {   
        serial_printf ("ERROR: Cannot obtain Socket Despcritor.\n");
        return ;
    } else {
        WEB_LOG ("OK: Obtain Socket Despcritor sucessfully.\n");
    }
    
    /* Fill the local socket address struct */
    addr_local.sin_family = AF_INET;           // Protocol Family
    addr_local.sin_port = htons(PORT);         // Port number
    addr_local.sin_addr.s_addr  = INADDR_ANY;  // AutoFill local address
    memset(&(addr_local.sin_zero), 0, 8);          // Flush the rest of struct

    /*  Blind a special Port */
    if( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1 ) {  
        serial_printf ("ERROR: Cannot bind Port %d\n.",PORT);
        return ;
    } else {
        WEB_LOG ("OK: Bind the Port %d sucessfully.\n",PORT);
    }
   
    /*  Listen remote connect/calling */
    if(listen(sockfd,BACKLOG) == -1)    {  
        serial_printf ("ERROR: Cannot listen Port %d\n.", PORT);
        return ;
    } else {
        WEB_LOG ("OK: Listening the Port %d sucessfully.\n", PORT);
    }
   
    while(1) {  
        sin_size = sizeof(struct sockaddr_in);  
        
        /*  Wait a connection, and obtain a new socket file despriptor for single connection */
        if ((nsockfd = accept(sockfd, (struct sockaddr *)&addr_remote, &sin_size)) == -1) {  
            serial_printf ("ERROR: Obtain new Socket Despcritor error\n");
            continue;
        } else {
            WEB_LOG ("OK: Server has got connect from %s\n", inet_ntoa(addr_remote.sin_addr)); 
        }
        
        num = recv(nsockfd, revbuf, LENGTH, 0);
        revbuf[num] = '\0';
        WEB_LOG (revbuf);
        
        /* Child process */
//        if(!fork())                    
        {  
            WEB_LOG ("OK: Http web is servering.\n");

            if(revbuf[5]==' ') {

                send(nsockfd, reply, sizeof(reply), 0); 
                send(nsockfd, html, sizeof(html), 0); 
            } else if(revbuf[5]=='1') {

                send(nsockfd, httpgif, sizeof(httpgif), 0);
                send(nsockfd, bmp, sizeof(bmp), 0);
            } else {
                char file_name[PATH_LEN_MAX] = {WEB_PATH};
                char file_context[1024];
                char send_context[2048];
                int  i = 5;
                int  fd;
                int  read_byte;
                int  send_byte;
#if 0
                int  f_len, h_len = strlen(HOME_HTML);
#endif
                while (!isspace(revbuf[i++]))
                    ;
                revbuf[--i] = '\0';
                strncat (file_name, revbuf + 5, PATH_LEN_MAX - strlen(file_name) - 1);
                file_name[PATH_LEN_MAX - 1] = '\0';
                
#if 0
                f_len = strlen (file_name);
                if ((f_len > h_len) &&
                    (strcmp (file_name + f_len - h_len, HOME_HTML) == 0)) {
                    strcpy (file_name, WEB_PATH HOME_HTML);
                }
#endif

                fd = yaffs_open(file_name, O_RDONLY, 0);
                if ( fd ==  -1) {
                    serial_printf("ERROR: Cannot open file %s", file_name);
                }

                if ( (strcmp (strchr (file_name, '.'), ".html") == 0) || /* *.html */
                     (strcmp (strchr (file_name, '.'), ".jpg") == 0)) { /* *.jpg */
                    read_byte = yaffs_read(fd, file_context, 1024);
                    while (read_byte > 0) {
                        send(nsockfd, file_context, read_byte, 0);
                        read_byte = yaffs_read(fd, file_context, 1024);
                    }
                } else { /* !.html */

                send(nsockfd, html_start, sizeof(html_start), 0); 
                read_byte = yaffs_read(fd, file_context+1, 1023);
                file_context[0] = '|';
                read_byte++;
                while (read_byte > 1) {
                    send_byte = 0;
                    for (i = 0; i < read_byte; i++) {
                        if (file_context[i] == '\n') {
                            send_context[send_byte++] = '<';
                            send_context[send_byte++] = 'B';
                            send_context[send_byte++] = 'R';
                            send_context[send_byte++] = '>';
                            send_context[send_byte++] = '|';
                        } else if (file_context[i] == '<'){
                            send_context[send_byte++] = '&';
                            send_context[send_byte++] = 'l';
                            send_context[send_byte++] = 't';
                        } else if (file_context[i] == '&'){
                            send_context[send_byte++] = '&';
                            send_context[send_byte++] = 'a';
                            send_context[send_byte++] = 'm';
                            send_context[send_byte++] = 'p';
                        } else if (file_context[i] == '"'){
                            send_context[send_byte++] = '&';
                            send_context[send_byte++] = 'q';
                            send_context[send_byte++] = 'u';
                            send_context[send_byte++] = 'o';
                            send_context[send_byte++] = 't';
                        }else {
                            send_context[send_byte++] = file_context[i];
                        }
                    }

                    send(nsockfd, send_context, send_byte, 0); 

                    read_byte = yaffs_read(fd, file_context, 1024);
                }
                send(nsockfd, html_end, sizeof(html_end), 0); 

                } /* !*.html */

                yaffs_close(fd);
            }
        }      
        close(nsockfd);  
//        while(waitpid(-1, NULL, WNOHANG) > 0);
    }    
}

/*==============================================================================
** FILE END
==============================================================================*/

