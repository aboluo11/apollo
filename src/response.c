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

int append_res_line(conn_t* conn){
    request_t* request = &conn->request;
    buffer_t* buffer = &request->ob;
    char* data = "HTTP/1.1 200 OK\r\n";
    append_out_buffer(buffer, data);
    request->action = append_res_header;
    return OK;
}

int append_res_header(conn_t* conn){
    request_t* request = &conn->request;
    buffer_t* buffer = &request->ob;
    char* data = "Content-Length: ";
    int len = request->content_length;
    char* s_len = (char*)malloc(13);
    sprintf(s_len, "%d\r\n\r\n", len);
    data = str_cat(data, s_len);
    free(s_len);
    append_out_buffer(buffer, data);
    free(data);
    request->action = send_buffer;
    return OK;
}

int send_buffer(conn_t* conn){
    request_t* request = &conn->request;
    buffer_t* buffer = &request->ob;
    int need_to_send = buffer->end - buffer->free - buffer->pos;
    while(1){
        int n = send(conn->fd, buffer->pos, need_to_send, 0);
        if(n > 0){
            buffer->pos += n;
            need_to_send -= n;
            if(need_to_send) continue;
            request->action = send_file;
            return OK;
        }else if(n == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                return AGAIN;
            }
            return ERROR;
        }
    }
}

int send_file(conn_t* conn){
    request_t* request = &conn->request;
    while(1){
        int n = sendfile(conn->fd, request->file_fd, NULL, request->content_length);
        if(n >= 0){
            request->content_length -= n;
            if(request->content_length) continue;
            request->action = NULL;
            close(request->file_fd);
            return OK;
        }else if(n == -1){
            if(errno == EAGAIN){
                return AGAIN;
            }
            return ERROR;
        }
    }
}

int get_file_info(conn_t* conn){
    request_t* request = &conn->request;
    char* root = "/root/share/apollo/www";
    char* location = request->uri_start;
    char* path = str_cat(root, location);
    int file_fd = open(path, O_RDONLY);
    if(file_fd == -1) return ERROR;
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    if(S_ISDIR(file_stat.st_mode)){
        char* new_path = str_cat(path, "index.html");
        close(file_fd);
        free(path);
        path = new_path;
        file_fd = open(path, O_RDONLY);
        if(file_fd == -1) return ERROR;
        fstat(file_fd, &file_stat);
    }
    request->file_fd = file_fd;
    request->content_length = file_stat.st_size;
    request->action = append_res_line;
    return OK;
}

int handle_response(conn_t* conn){
    request_t* request = &conn->request;
    int status;
    do{
        status = request->action(conn);
    }while(status == OK && request->action);
    return status;
}