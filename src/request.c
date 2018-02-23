#include "server.h"
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

int read_in_stream(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* buffer = request->ib;
    if(buffer->end - buffer->free != buffer->pos){
        return buffer->end - buffer->free;
    }
    int n = recv(conn->fd, buffer->end - buffer->free, buffer->free, 0);
    if(n == 0) return ERROR;
    if(n == -1){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return AGAIN;
        }else{
            return ERROR;
        }
    }
    buffer->free -= n;
    return n;
}

int header_conn_handler(request_t* request){
    if(strcmp(request->header_value, "keep-alive") == 0){
        request->keep_alive = 1;
        return OK;
    }else if(strcmp(request->header_value, "close") == 0){
        request->keep_alive = 0;
        return OK;
    }
    return ERROR;
}

int handle_request_header(conn_t* conn){
    request_t* request = conn->request;
    header_handler = get_dict(header_dict, request->header_key);
    if(handler){
        return header_handler(request);
    }else{
        return OK;
    }
}

int handle_request(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* buffer = request->ib;
    while(1){
        int n = read_in_stream(conn);
        if(n == AGAIN || n == ERROR) return n;
        int status = request->action(conn);
        if(status == ERROR) return ERROR;
        else if(status == OK){
            change_to_response(conn);
            return OK;
        }else if(status == AGAIN){
            if(buffer->free == 0){
                realloc_ib(conn);
            }
        }
    }
}

void reset_request(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* ib = request->ib;
    if(ib->end - ib->free != ib->pos){
        handle_request(conn);
    }else{
        free_conn(conn);
        conn->pool = init_pool();
        conn->request = init_request(conn->pool);
    }
}