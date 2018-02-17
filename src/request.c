#include "server.h"
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

int read_in_stream(conn_t* conn){
    request_t* request = &conn->request;
    buffer_t* buffer = &request->ib;
    while(1){
        int n;
        ensure_ib_available(request);
        n = recv(conn->fd, buffer->end - buffer->free, buffer->free, 0);
        if(n == 0) return ERROR;
        if(n == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                return AGAIN;
            }else return ERROR;
        }
        buffer->free -= n;
    }
}

int handle_request(conn_t* conn){
    request_t* request = &conn->request;
    int status = read_in_stream(conn);
    if(status == ERROR) return ERROR;
    do{
        status = request->action(conn);
    }while(status == OK && request->action);
    if(status == OK){
        change_to_response(conn);
    } 
    return status;
}