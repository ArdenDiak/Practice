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

/* Max cache and object sizes */
#define MAX_CACHE_SIZE (1024 * 1024)
#define MAX_OBJECT_SIZE (100 * 1024)

#define MAX_URI_LEN  100

struct cache_block{
    char *uri;
    size_t uri_len;

    char *data;
    size_t data_len;

    int time_since_last_access;

    struct cache_block *next;
    struct cache_block *prev;
};
typedef struct cache_block block_t;

typedef struct {
    block_t *head;
    block_t *tail;
    int     num_blocks;
    size_t  total_cache_size;
} cache_t;

cache_t *cache_new(void);

void cache_free(cache_t *cache);

int cache_store(cache_t *cache,
                 char *uri,
                 size_t uri_len,
                 char *data,
                 size_t data_len);

// returns NULL if no such block found
block_t *cache_lookup(cache_t *cache, char *uri);

// Internal helper function: evicts a block from the cache (cache loses a block)
void cache_evict(cache_t *cache);

// returns the least-recently-used block
block_t *get_lru_block(cache_t *cache);
