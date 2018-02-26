#include "server.h"
#include <stdlib.h>
#include <string.h>

buffer_t* buffer_init(pool_t* pool){
	buffer_t* buffer = pool_alloc(pool, sizeof(buffer_t));
	buffer->pos = pool_alloc(pool, BUFFER_SIZE);
	buffer->end = buffer->pos + BUFFER_SIZE;
	buffer->free = BUFFER_SIZE;
    buffer->next = NULL;
	return buffer;
}

void ib_realloc(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* old_buf = request->ib;
    buffer_t* new_buf = buffer_init(request->pool);
    char* cp_start;
    int cp_len;
    request->ib = new_buf;
    if(request->need_to_copy == 0) return;
    if(request->action == parse_request_line){
        cp_start = request->uri_start;
        request->uri_start = new_buf->pos;
    }else{
        cp_start = request->header_key;
        int key_value_diff = request->header_value - request->header_key;
        request->header_key = new_buf->pos;
        request->header_value = request->header_key + key_value_diff;
    }
    cp_len = old_buf->end - cp_start;
    memcpy(new_buf->pos, cp_start, cp_len);
    new_buf->pos += cp_len;
    new_buf->free -= cp_len;
}

void ob_realloc(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* old_buf = request->ob;
    while(old_buf->next){
        old_buf = old_buf->next;
    }
    buffer_t* new_buf = buffer_init(request->pool);
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
            int send = len > buffer->free ? buffer->free : len;
            memcpy(buffer->end - buffer->free, data + sent, send);
            sent += send;
            len -= send;
            buffer->free -= send;
        }else{
            ob_realloc(conn);
            buffer = buffer->next;
        }
    }
}