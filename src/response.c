#include "server.h"       
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/sendfile.h>

void append_res_line(conn_t* conn){
    char* data;
    request_t* request = conn->request;
    if(request->status_code == 200){
        data = "HTTP/1.1 200 OK\r\n";
    }else if(request->status_code == 404){
        data = "HTTP/1.1 404 Not Found\r\n";
    }
    append_out_buffer(conn, data);
}

void append_content_length_header(conn_t* conn){
    int len = conn->request->content_length;
    char* data = pool_alloc(conn->pool, 29);
    sprintf(data, "Content-Length: %d\r\n", len);
    append_out_buffer(conn, data);
}

void append_connection_header(conn_t* conn){
    int keep_alive = conn->request->keep_alive;
    char* data;
    if(conn->request->keep_alive) data = "keep-alive\r\n";
    else data = "close\r\n";
    data = str_cat(conn, "Connection: ", data);
    append_out_buffer(conn, data);
}

void append_CRLF_header(conn_t* conn){
    append_out_buffer(conn, "\r\n");
}

void append_res_header(conn_t* conn){
    append_content_length_header(conn);
    append_connection_header(conn);
    append_CRLF_header(conn);
}

int send_one_buffer(conn_t* conn){
    buffer_t* buffer = conn->request->ob;
    int need_to_send = buffer->end - buffer->free - buffer->pos;
    while(1){
        int n = send(conn->fd, buffer->pos, need_to_send, 0);
        if(n > 0){
            buffer->pos += n;
            need_to_send -= n;
            if(need_to_send == 0) return OK;
        }else if(n == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                return AGAIN;
            }
            return ERROR;
        }
    }
}

int send_buffer(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* buffer = request->ob;
    while(buffer){
        int status = send_one_buffer(conn);
        if(status != OK) return status;
        buffer = buffer->next;
    }
    request->action = send_file;
    return send_file(conn);
}

int send_file(conn_t* conn){
    request_t* request = conn->request;
    while(request->content_length){
        int n = sendfile(conn->fd, request->file_fd, NULL, request->content_length);
        if(n > 0){
            request->content_length -= n;
        }else if(n == -1){
            if(errno == EAGAIN){
                return AGAIN;
            }
            return ERROR;
        }
    }
    if(request->keep_alive){
        request_reset(conn);
        change_to_request(conn);
        return OK;
    }else{
        return ERROR;
    }
}

void get_file_info(conn_t* conn){
    request_t* request = conn->request;
    char* root = "/root/share/apollo/www";
    char* path = str_cat(conn, root, request->uri_start);
    int file_fd = open(path, O_RDONLY);
    if(file_fd == -1){
        request->status_code = 404;
        request->uri_start = "/404.html";
        get_file_info(conn);
    }else{
        struct stat file_stat;
        fstat(file_fd, &file_stat);
        if(S_ISDIR(file_stat.st_mode)){
            request->uri_start = str_cat(conn, request->uri_start, "index.html");
            get_file_info(conn);
            return;
        }
        request->file_fd = file_fd;
        request->content_length = file_stat.st_size;
    }
}

int handle_response(conn_t* conn){
    request_t* request = conn->request;
    return request->action(conn);
}