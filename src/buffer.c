#include "server.h"
#include <stdlib.h>
#include <string.h>

buffer_t* create_buf(conn_t* conn){
    buffer_t* buffer = alloc_pool(conn->pool, sizeof(buffer_t));
    buffer->pos = alloc_pool(conn->pool, BUFFER_SIZE);
    buffer->end = buffer->pos + BUFFER_SIZE;
    buffer->free = BUFFER_SIZE;
    buffer->next = NULL;
    return buffer;
}

void realloc_ib(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* old_buf = request->ib;
    buffer_t* new_buf = create_buf(conn);
    char* cp_start;
    int cp_len;
    request->ib = new_buf;
    if(request->need_to_copy == 0) return;
    if(request->action == parse_request_line){
        cp_start = request->uri_start;
        request->uri_start = new_buf->pos;
    }else{
        cp_start = header_key;
        int key_value_diff = request->header_value - request->header_key;
        request->header_key = new_buf->pos;
        request->header_value = request->header_key + key_value_diff;
    }
    cp_len = old_buf->end - cp_start;
    memcpy(new_buf->pos, cp_start, cplen);
    new_buf->pos += cp_len;
    new_buf->free -= cp_len;
}

void realloc_ob(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* old_buf;
    while(old_buf->next){
        old_buf = old_buf->next;
    }
    buffer_t* new_buf = create_buf(conn);
    old_buf->next = new_buf;
}

void append_out_buffer(conn_t* conn, char* data){
    int len = strlen(data);
    buffer_t* buffer = conn->request->ob;
    while(buffer->next){
        buffer = buffer->next;
    }
    int sent = 0;
    while(len){
        if(buffer->free){
            int send = len > free ? free : len;
            memcpy(buffer->pos, data + sent, send);
            sent += send;
            len -= send;
            buffer->free -= send;
        }else{
            realloc_ob(conn);
            buffer = buffer->next;
        }
    }
}