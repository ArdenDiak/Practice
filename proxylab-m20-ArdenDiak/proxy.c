/*
 * NOTES:
 *    - A socket can only TX/RX MAXLINE bytes at a time,
 *      if the file is larger than MAXLINE bytes, then it will have to be sent out or received in chunks!
 */


#include "socket_comm.h"
#include "cache.h"
#include "csapp.h"

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

pthread_mutex_t mutex;
cache_t *cache;

void *handle_cli(void *argp);


int main(int argc, char **argv) {

    int pxy_listenfd;

    // Client socket address
    struct sockaddr_in cli_addr;
    socklen_t cli_addrlen;

    pthread_t tid;
    pthread_mutex_init(&mutex, NULL);

    cache = cache_new();

    signal(SIGPIPE, SIG_IGN);

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    pxy_listenfd = open_listenfd(argv[1]);
    if (pxy_listenfd < 0) {
        fprintf(stderr, "Failed to listen on port: %s\n", argv[1]);
        exit(1);
    }


    int *cli_connfd;
    while(1) {
        cli_connfd = malloc(sizeof(int));
        *cli_connfd = accept(pxy_listenfd, (struct sockaddr*) &cli_addr, &cli_addrlen);
        if (*cli_connfd < 0){
            fprintf(stderr, "Failed to accept connex REQ on port: %s\n", argv[1]);
            cache_free(cache);
            free(cli_connfd);
            exit(1);
        }

        pthread_create(&tid, NULL, handle_cli, (void*)cli_connfd);
    }

    close(pxy_listenfd);
    cache_free(cache);
    pthread_exit(NULL);
}

void *handle_cli(void *argp) {
    int cli_connfd, pxy_connfd;

    // Client Requests and EOF
    char cli_get_req[MAXLINE];
    rio_t rio_cli_connfd;
    char cli_req_hdrs[MAXLINE * MAX_NUM_CLI_HDRS];

    // Client requested URI (host, path)
    char cli_reqd_host[MAX_HOST_LEN];
    char cli_reqd_path[MAX_PATH_LEN];
    char cli_reqd_port[MAX_PORT_LEN];
    char cli_reqd_uri[MAX_URI_LEN];

    //Proxy request sent to srv
    char pxy_req[MAX_REQ_LEN];
    char pxy_replace_host_hdr[MAX_HOST_LEN] = {0};

    // Srv response to proxy (for caching)
    char *srv_resp;
    char *cache_reqd_uri;
    char srv_resp_buf[MAX_OBJECT_SIZE] = {0};

    cli_connfd = *((int*)argp);
    free(argp);
    pthread_detach(pthread_self());

    rio_readinitb(&rio_cli_connfd, cli_connfd);

    int req_parse_result;
    req_parse_result = read_req_line_parser(cli_connfd,
                         &rio_cli_connfd,
                         cli_get_req,
                         cli_req_hdrs,
                         pxy_replace_host_hdr,
                         cli_reqd_host, cli_reqd_path, cli_reqd_port,
                         cli_reqd_uri);

    // close client connection on malformed requests
    if(req_parse_result < 0){
        fprintf(stderr, "Malformed request\n");
        close(cli_connfd);
        pthread_exit(NULL);
    }

    printf("cli ---> pxy      srv:\n%s\n" \
           "======================\n", cli_req_hdrs);

    // check in cache
    block_t *cache_block;

    pthread_mutex_lock(&mutex);
    cache_block = cache_lookup(cache, cli_reqd_uri);
    pthread_mutex_unlock(&mutex);

    // Requested URI found in cache, serve from cache
    if(cache_block != NULL){
        printf("Lookup for %s returned FOUND\n", cli_reqd_uri);

        if(rio_writen(cli_connfd, cache_block->data, cache_block->data_len) < 0){
            fprintf(stderr, "Error forwarding client's request to server\n");
            pthread_exit(NULL);
        }

        close(cli_connfd);
        pthread_exit(NULL);
    }


    // Not found in cache, so must querry server
    pxy_connfd = open_clientfd(cli_reqd_host, cli_reqd_port);
    if(pxy_connfd < 0){
        fprintf(stderr, "Failed to connect to %s:%s\n", cli_reqd_host, cli_reqd_port);
        close(cli_connfd);
        pthread_exit(NULL);
    }

    // pxy <--> srv connection established
    printf("cli      pxy <--> srv @ %s:%s\n" \
           "=======================\n", cli_reqd_host, cli_reqd_port);

   form_request(pxy_req,
                cli_reqd_host,
                cli_reqd_port,
                cli_reqd_path,
                cli_req_hdrs,
                 pxy_replace_host_hdr);


    // write request to server
    pxy_tx(pxy_connfd, pxy_req, MAX_REQ_LEN);

    printf("cli      pxy ---> srv:\n%s\n" \
           "=====================\n",pxy_req);

    ssize_t bytes_read = 0;
    size_t total_bytes_read = 0;
    rio_t server_rio;
    rio_readinitb(&server_rio, pxy_connfd);
    char srv_resp_line[MAXLINE];
    bool data_obj_cacheable = true;

    // Read response from req'd server
    while((bytes_read = rio_readnb(&server_rio, srv_resp_line, MAXLINE)) > 0){

        // write response to client
        if(rio_writen(cli_connfd, srv_resp_line, bytes_read) < 0){
            fprintf(stderr, "error writing response to client\n");
            pthread_exit(NULL);
        }

        if((total_bytes_read + bytes_read) <= MAX_OBJECT_SIZE) {
            memcpy((void*)((char*)srv_resp_buf + total_bytes_read), srv_resp_line, bytes_read);
            total_bytes_read += bytes_read;
        } else {
            data_obj_cacheable = false;
        }
    }
    if(bytes_read < 0){
        fprintf(stderr, "Error reading %d bytes from %s\n", (int)MAXLINE, cli_reqd_host);
        pthread_exit(NULL);
    }

    if(total_bytes_read < MAX_OBJECT_SIZE &&
       data_obj_cacheable) {
        printf("Caching data for URI: %s\n", cli_reqd_uri);
        srv_resp = malloc(MAX_OBJECT_SIZE * sizeof(char)); // freed by cache_fee()
        memcpy(srv_resp, srv_resp_buf, total_bytes_read);

        cache_reqd_uri = malloc(MAX_URI_LEN * sizeof(char));
        strncpy(cache_reqd_uri, cli_reqd_uri, MAX_URI_LEN); // freed by cache_fee()

        pthread_mutex_lock(&mutex);
        cache_store(cache,
                    cache_reqd_uri,
                    strlen(cache_reqd_uri),
                    srv_resp,
                    total_bytes_read);
        pthread_mutex_unlock(&mutex);
    }else{
        printf("URI: %s has %zu bytes too big to cache\n", cli_reqd_uri, total_bytes_read);
    }

    close(pxy_connfd);
    close(cli_connfd);
    pthread_exit(NULL);
}
