#include <string.h>

chunk_t* init_chunk(){
    chunk_t* chunk = malloc(CHUNK_SIZE);
    chunk->start = chunk;
    chunk->pos = chunk + sizeof(chunk_t);
    chunk->next = NULL;
    return chunk;
}

pool_t* init_pool(){
    chunk_t* chunk = init_chunk();
    pool_t* pool = chunk->pos;
    chunk->pos += sizeof(pool_t);
    pool->first = chunk;
    pool->current = chunk;
    return pool;
}

void realloc_pool(pool_t* pool){
    chunk_t* new_chunk = init_chunk();
    chunk_t* old_chunk = pool->current;
    old_chunk->next = new_chunk;
    pool->current = new_chunk;
}

void* alloc_pool(pool_t* pool, int size){
    if(size > CHUNK_SIZE) return NULL;
    chunk_t* chunk = pool->current;
    int free = chunk->start + CHUNK_SIZE - chunk->pos;
    if(size <= free){
        chunk->pos += size;
        return chunk->pos - size;
    }else{
        realloc_pool(pool);
        return alloc_pool(pool, size);
    }
}

void* calloc_pool(pool_t* pool, int size){
    void* p = alloc_pool(pool, size);
    memset(p, 0, size);
    return p;
}