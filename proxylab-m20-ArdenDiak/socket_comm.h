#include "csapp.h"
#include "http_parser.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>


// maximum size of the "Host: ..." HTTP request header
#define MAX_HOST_LEN 60
#define MAX_PATH_LEN 30
#define MAX_PORT_LEN 8

// maximum size of GET Resquest (including hdrs)
#define MAX_NUM_HDRS 14
#define MAX_REQ_LEN (MAXLINE * MAX_NUM_HDRS)

// maximum size of server response
#define MAX_RESP_LEN MAXLINE * 10

// maximum number of addt'l headers the client can include (cli --> pxy)
#define MAX_NUM_CLI_HDRS 10

// maxium length of an HTTP request header's name
#define MAX_HDR_NAME_LEN 30

// length of an EOF message
#define EOF_LEN 10

/* Ported from Tiny Web Server */
void clienterror(int fd,
                 const char *errnum,
                 const char *shortmsg,
                 const char *longmsg);

/* Reads headers from client-supplied request line */
void read_req_line( int cli_connfd,
                    rio_t *rio_cli_req,
                    char *cli_get_req,
                    char *cli_req_hdrs,
                    char *pxy_replace_host_hdr);

int read_req_line_parser( int cli_connfd,
                           rio_t *rio_cli_req,
                           char *cli_get_req,
                           char *cli_req_hdrs,
                           char *pxy_replace_host_hdr,
                           char *host,
                           char *path,
                           char *port,
                           char *uri);


/* Create a proper GET request from client-supplied headers*/
void form_request(char *pxy_req,
             char *cli_reqd_host,
             char *cli_reqd_port,
             char *cli_reqd_path,
             char *cli_req_hdrs,
             char *pxy_replace_host_hdr);

/* Parses client-supplied URI from GET request */
void parse_req(int cli_connfd,
               char *req_line,
               char *host,
               char *path,
               char *port);

/* RX/TX Wrapper functions for proxy sockets communication */
void pxy_rx(int sockfd,
            char *data_buf,
            size_t data_len);

void pxy_tx(int sockfd,
            char *data,
            size_t data_len);
