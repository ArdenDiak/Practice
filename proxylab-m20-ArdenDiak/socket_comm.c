#include "socket_comm.h"

/* Ported from Tiny Web Server */
void clienterror(int fd,
                 const char *errnum,
                 const char *shortmsg,
                 const char *longmsg)
{
    char buf[MAXLINE];
    char body[MAXBUF];
    size_t buflen;
    size_t bodylen;

    /* Build the HTTP response body */
    bodylen = snprintf(body, MAXBUF,
            "<!DOCTYPE html>\r\n" \
            "<html>\r\n" \
            "<head><title>Tiny Error</title></head>\r\n" \
            "<body bgcolor=\"ffffff\">\r\n" \
            "<h1>%s: %s</h1>\r\n" \
            "<p>%s</p>\r\n" \
            "<hr /><em>The Tiny Web server</em>\r\n" \
            "</body></html>\r\n", \
            errnum, shortmsg, longmsg);
    if (bodylen >= MAXBUF) {
        return; // Overflow!
    }

    /* Build the HTTP response headers */
    buflen = snprintf(buf, MAXLINE,
            "HTTP/1.0 %s %s\r\n" \
            "Content-Type: text/html\r\n" \
            "Content-Length: %zu\r\n\r\n", \
            errnum, shortmsg, bodylen);
    if (buflen >= MAXLINE) {
        return; // Overflow!
    }

    /* Write the headers */
    if (rio_writen(fd, buf, buflen) < 0) {
        fprintf(stderr, "Error writing error response headers to client\n");
        return;
    }

    /* Write the body */
    if (rio_writen(fd, body, bodylen) < 0) {
        fprintf(stderr, "Error writing error response body to client\n");
        return;
    }
}

/* Reads headers from client-supplied request line */
void read_req_line( int cli_connfd,
                    rio_t *rio_cli_req,
                    char *cli_get_req,
                    char *cli_req_hdrs,
                    char *pxy_replace_host_hdr)
{

    char req_line[MAXLINE];
    do{
        if (rio_readlineb(rio_cli_req, req_line, MAXLINE) <= 0){
            fprintf(stderr, "Error reading REQ from client\n");
            return;
        }

        /* Extract headers from the recieved request line */
        char *hdr;
        char *req_line_str_loc;
        int i=0;
        hdr = strtok_r(req_line, "\r\n", &req_line_str_loc);
        while((hdr != NULL) &&
              (strcmp(hdr,"\r\n") != 0))
        {
            if (strncmp(hdr, "GET", 3) == 0) {
                strncpy(cli_get_req, hdr, MAXLINE);
            }
            else if(strncmp(hdr, "Host", 4) == 0) {
                strncpy(pxy_replace_host_hdr, hdr, MAXLINE);

            // save any client headers to fwd them later
            }else if((strncmp(hdr, "Connection", 10) != 0) &&
                     (strncmp(hdr, "User-Agent", 10) != 0) &&
                     (strncmp(hdr, "Proxy-Connection", 16) != 0)) {

                strncat(cli_req_hdrs, hdr, MAXLINE);
                strcat(cli_req_hdrs, "\r\n");
            }

            // get next header
            hdr = strtok_r(NULL, "\r\n", &req_line_str_loc);
            i++;
        }
    }while(strcmp(req_line, "\r\n") != 0);
}

int read_req_line_parser( int cli_connfd,
                           rio_t *rio_cli_req,
                           char *cli_get_req,
                           char *cli_req_hdrs,
                           char *pxy_replace_host_hdr,
                           char *host,
                           char *path,
                           char *port,
                           char *uri)
{
    char req_line[PARSER_MAXLINE];
    parser_t *parser;
    parser_state state;

    parser = parser_new();

    do{
        rio_readlineb(rio_cli_req, req_line, PARSER_MAXLINE);
        state = parser_parse_line(parser, req_line);

        if(state == REQUEST){ //must be a REQUEST line
            const char *val[30];
            char *tmp= " ";
            val[0]= tmp;

            parser_retrieve(parser, METHOD, val);
            if (strcmp(*val, "GET") != 0) {
                clienterror(cli_connfd, "501", "Not Implemented",
                            "This request type is not implemented.");
                return -1;
            }

            parser_retrieve(parser, HOST, val);
            const char *req_host= *val;
            parser_retrieve(parser, PORT, val);
            const char *req_port= *val;
            parser_retrieve(parser, PATH, val);
            const char *req_path= *val;
            parser_retrieve(parser, URI, val);
            const char *req_uri= *val;

            strcpy(host, req_host);
            strcpy(port, req_port);
            strcpy(path, req_path);
            strcpy(uri,  req_uri);
        }
        else if(state == ERROR) return -1;

    }while(strcmp(req_line, "\r\n") != 0);

    header_t *hdr;
    char tmp_fmt_hdr[MAXLINE];
    hdr = parser_retrieve_next_header(parser);
    while(hdr != NULL){
        if(strcmp(hdr->name, "Host") == 0) {
            snprintf(pxy_replace_host_hdr, MAXLINE, "%s", hdr->value);

        } else if((strcmp(hdr->name, "User-Agent") != 0) &&
                  (strcmp(hdr->name, "Connection") != 0) &&
                  (strcmp(hdr->name, "Proxy-Connection") != 0)) {

            snprintf(tmp_fmt_hdr, MAXLINE, "%s: %s\r\n", hdr->name, hdr->value);
            strncat(cli_req_hdrs, tmp_fmt_hdr, MAXLINE);
        }

        // get next header
        hdr = parser_retrieve_next_header(parser);
    }

    parser_free(parser);
    return 0;
}



/* Create a proper GET request from client-supplied headers*/
void form_request(char *pxy_req,
             char *cli_reqd_host,
             char *cli_reqd_port,
             char *cli_reqd_path,
             char *cli_req_hdrs,
             char *pxy_replace_host_hdr)
{

    /*
     * String to use for the User-Agent header.
     * Don't forget to terminate with \r\n
     */
    static const char *header_user_agent = "Mozilla/5.0"
                                           " (X11; Linux x86_64; rv:3.10.0)"
                                           " Gecko/20191101 Firefox/63.0.1";

    char final_cli_reqd_host[MAX_HOST_LEN];
    if(pxy_replace_host_hdr[0] != 0){
        //the "Host: ..." HTTP header contains the port no. too
        snprintf(final_cli_reqd_host, MAX_HOST_LEN, "%s", pxy_replace_host_hdr);
    }else{
        snprintf(final_cli_reqd_host, MAX_HOST_LEN, "%s:%s", cli_reqd_host, cli_reqd_port);
    }

    snprintf(pxy_req, MAX_REQ_LEN,
             "GET %s HTTP/1.0\r\n" \
             "Host: %s\r\n" \
             "User-Agent: %s\r\n" \
             "Connection: close\r\n" \
             "Proxy-Connection: close\r\n" \
             "%s"
             "\r\n",
             cli_reqd_path,
             final_cli_reqd_host,
             header_user_agent,
             cli_req_hdrs);
}

/* Parses client-supplied URI from GET request */
void parse_req(int cli_connfd,
               char *req_line,
               char *host,
               char *path,
               char *port)
{
    /* Parse the request line and check if it's well-formed */
    char method[MAXLINE];
    char uri[MAXLINE];
    char version;

    /* sscanf must parse exactly 3 things for request line to be well-formed */
    /* version must be either HTTP/1.0 or HTTP/1.1 */
    if (sscanf(req_line, "%s %s HTTP/1.%c", method, uri, &version) != 3
            || (version != '0' && version != '1')) {
        clienterror(cli_connfd, "400", "Bad Request",
                    "Tiny received a malformed request");
        return;
    }

    /* Check that the method is GET (Proxy only supports GET requests) */
    if (strcmp(method, "GET") != 0) {
        clienterror(cli_connfd, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }

    //printf("method: %s\nuri: %s\n", method, uri);

    /* Retrieve host and path from URI */
    char *uri_str_loc;
    char *token = strtok_r(uri, "/", &uri_str_loc);
    int i=0;
    while(token != NULL){

        //printf("%d: %s\n", i, token);
        if(i == 1) {
            strncpy(host, token, MAX_HOST_LEN);
            snprintf(path, MAX_PATH_LEN, "/%s", uri_str_loc);
            break;
        }
        token = strtok_r(NULL, "/", &uri_str_loc);
        i++;
    }

    // retrieve the client-requested port number
    token = strtok_r(host, ":", &uri_str_loc);
    //printf("%s\n%d\n",token, *uri_str_loc);
    if(*uri_str_loc == 0)
        sprintf(port, "80");
    else
        strncpy(port, uri_str_loc, MAX_PORT_LEN);
    //printf(">> PORT: %s\n",port);
}

/* Proxy TX/RX */
void pxy_tx(int sockfd,
            char *data,
            size_t data_len)
{
    if (rio_writen(sockfd, data, data_len) < 0) {
        fprintf(stderr, "Error forwarding client's request to server\n");
        return;
    }
}

void pxy_rx(int sockfd,
            char *data_buf,
            size_t data_len)
{
    if (rio_readn(sockfd, data_buf, data_len) <= 0) {
        fprintf(stderr, "Error reading server's response\n");
        return;
    }
}
