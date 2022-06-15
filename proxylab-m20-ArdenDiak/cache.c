#include "cache.h"

cache_t *cache_new(void){
    cache_t *cache;
    cache = malloc(sizeof(cache_t));
    cache->head = NULL;
    cache->tail = NULL;
    cache->total_cache_size = sizeof(cache_t);
    cache->num_blocks = 0;
    return cache;
}

void cache_free(cache_t *cache)
{
    block_t *iter;
    iter = cache->head;
    while(iter != NULL) {
        free(iter->data);
        free(iter->uri);
        free(iter);
        iter = iter->next;
    }
    free(cache);
}

int cache_store(cache_t *cache,
                 char *uri,
                 size_t uri_len,
                 char *data,
                 size_t data_len)
{
    // multithreaded many tid's storing - store UNIQUE webpages
    if(cache_lookup(cache, uri) != NULL)
        return 1;

    cache->total_cache_size += (data_len + uri_len);
    cache->total_cache_size += sizeof(block_t);

    while(cache->total_cache_size >= MAX_CACHE_SIZE) {
        cache_evict(cache);
    }
    cache->num_blocks++;

    block_t *new_block;
    new_block = malloc(sizeof(block_t));

    new_block->data     = data;
    new_block->data_len = data_len;
    new_block->uri      = uri;
    new_block->uri_len  = uri_len;

    new_block->next     = NULL;
    new_block->time_since_last_access = 0;

    // empty cache
    if(cache->head == NULL){
        new_block->prev = NULL;
        cache->head = new_block;
        cache->tail = new_block;
        return 0;
    }

    // one element
    if(cache->head == cache->tail && cache->head != NULL){
        cache->head->next = new_block;
        new_block->prev = cache->head;
        cache->tail = new_block;

        cache->head->time_since_last_access++;
        return 0;
    }

    // more than 2 elements
    if(cache->head != cache->tail && cache->head != NULL){

        // increment all other blocks' last_accessed counters
        block_t *iter;
        iter = cache->head;
        while(iter != NULL) {
            iter->time_since_last_access++;
            iter = iter->next;
        }

        cache->tail->next = new_block;
        new_block->prev= cache->tail;
        cache->tail = new_block;

        return 0;
    }

    return 0;
}

block_t *cache_lookup(cache_t *cache, char *uri) {
    printf("Lookup for %s over %d blocks\n", uri, cache->num_blocks);
    block_t *iter, *located_block;
    iter = cache->head;
    located_block = NULL;
    while(iter != NULL) {
        if(strcmp(iter->uri, uri) == 0){
            iter->time_since_last_access = 0;
            located_block = iter;
        }

        iter->time_since_last_access++;
        iter = iter->next;
    }
    return located_block;
}

void cache_evict(cache_t *cache) {
    block_t *victim;
    victim = get_lru_block(cache);
    printf("freeeing victim %s\n", victim->uri);

    cache->total_cache_size -= (victim->data_len + victim->uri_len);
    cache->total_cache_size -= sizeof(block_t);
    cache->num_blocks       -= 1;

    free(victim->data);
    free(victim->uri);

    // cache mustn't be empty
    //@assert(cache->head != NULL);

    // one element (unusual case)
    if(cache->head == cache->tail && cache->head != NULL){
        cache->head = NULL;
        cache->tail = NULL;
        free(victim);
        return;
    }

    // 2 or more elements
    block_t *next_block;
    block_t *prev_block;
    if(cache->head != cache->tail && cache->head != NULL){
        if(victim == cache->head){
            next_block= victim->next;
            next_block->prev = NULL;
            cache->head = next_block;

        } else if(victim == cache->tail){
            prev_block= victim->prev;
            prev_block->next = NULL;
            cache->tail = prev_block;

        } else {
            prev_block = victim->prev;
            next_block = victim->next;
            next_block->prev = prev_block;
            prev_block->next = next_block;
        }
    }

    free(victim);
    return;
}

block_t *get_lru_block(cache_t *cache) {
    printf("cache has %d blocks\n", cache->num_blocks);
    block_t *iter, *lru_block;
    int max_accesses = 0;
    iter = cache->head;
    lru_block = cache->head;
    while(iter != NULL) {
        if(iter->time_since_last_access > max_accesses){
            printf(">> max_time_since_accessed: %d, lru_block's URI: %s\n", max_accesses, iter->uri);
            max_accesses = iter->time_since_last_access;
            lru_block = iter;
        }
        iter = iter->next;
    }
    return lru_block;
}
