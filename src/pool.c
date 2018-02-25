#include <string.h>
#include <stdlib.h>
#include "server.h"

chunk_t* chunk_init(){
    chunk_t* chunk = malloc(CHUNK_SIZE);
    chunk->start = (char*)chunk;
    chunk->pos = (char*)chunk + sizeof(chunk_t);
    chunk->next = NULL;
    return chunk;
}

pool_t* pool_init(){
    chunk_t* chunk = chunk_init();
    pool_t* pool = (pool_t*)chunk->pos;
    chunk->pos += sizeof(pool_t);
    pool->first = chunk;
    pool->current = chunk;
    return pool;
}

void pool_realloc(pool_t* pool){
    chunk_t* new_chunk = chunk_init();
    chunk_t* old_chunk = pool->current;
    old_chunk->next = new_chunk;
    pool->current = new_chunk;
}

void* pool_alloc(pool_t* pool, int size){
    if(size > CHUNK_SIZE) return NULL;
    chunk_t* chunk = pool->current;
    int free = chunk->start + CHUNK_SIZE - chunk->pos;
    if(size <= free){
        chunk->pos += size;
        return chunk->pos - size;
    }else{
        pool_realloc(pool);
        return pool_alloc(pool, size);
    }
}

void* pool_calloc(pool_t* pool, int size){
    void* p = pool_alloc(pool, size);
    memset(p, 0, size);
    return p;
}

void pool_free(conn_t* conn){
	chunk_t* chunk = conn->pool->first;
	chunk_t* next;
	while(chunk){
		next = chunk->next;
		free(chunk);
		chunk = next;
	}
}