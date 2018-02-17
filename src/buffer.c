#include "server.h"
#include <stdlib.h>
#include <string.h>

void realloc_ib(request_t* request){
    buffer_t* buffer = &request->ib;
    char* old_buf = buffer->start;
    int old_len = buffer->end - buffer->start;
    int new_len = old_len * 2;
    char* new_buf = (char*)malloc(new_len);
    int diff = new_buf - old_buf;
    memcpy(new_buf, old_buf, old_len);
    buffer->pos = buffer->pos + diff;
    buffer->start = new_buf;
    buffer->end = new_buf + new_len;
    buffer->free = old_len;
    request->uri_start += diff;
    request->uri_end += diff;
    free(old_buf);
}

void ensure_ib_available(request_t* request){
    buffer_t* buffer = &request->ib;
    if(buffer->free) return;
    realloc_ib(request);
}

void realloc_ob(buffer_t* buffer){
    char* old_buf = buffer->start;
    int old_len = buffer->end - buffer->start;
    char* new_buf = (char*)malloc(old_len*2);
    int diff = new_buf - old_buf;
    memcpy(new_buf, old_buf, buffer->pos - buffer->start);
    buffer->start = new_buf;
    buffer->end = new_buf + old_len * 2;
    buffer->pos += diff;
    buffer->free += old_len;
    free(old_buf);
}

int append_out_buffer(buffer_t* buffer, char* data){
    int len = strlen(data);
    while(buffer->free < len){
        realloc_ob(buffer);
    }
    memcpy(buffer->end - buffer->free, data, len);
    buffer->free -= len;
}