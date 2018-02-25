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
        return buffer->end - buffer->free - buffer->pos;
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

int header_conn_handler(conn_t* conn){
    request_t* request = conn->request;
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
    header_handler handler = dict_get(header_dict, request->header_key);
    if(handler){
        return handler(conn);
    }else{
        return OK;
    }
}

int handle_request(conn_t* conn){
    if(!conn->pool){
        conn->pool = pool_init();
        conn->request = request_init(conn->pool);
    }
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
                ib_realloc(conn);
            }
        }
    }
}

void request_reset(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* ib = request->ib;
    if(ib->end - ib->free != ib->pos){
        handle_request(conn);
    }else{
        pool_free(conn);
        conn->pool = NULL;
    }
}