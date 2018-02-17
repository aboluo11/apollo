#include "server.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

int start_listen(){
    int listen_fd;
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		return ERROR;
	}
    if(bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
		return ERROR;
	}
    if(listen(listen_fd, 1024) < 0){
		return ERROR;
	}
    return listen_fd;
}

void epoll_add_listen_fd(int listen_fd, int epfd){
	struct epoll_event event;
	int* ptr = (int*)malloc(sizeof(int));
	*ptr = listen_fd;
	event.events = EPOLLIN;
	event.data.ptr = ptr;
	epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event);
}

void set_fd_non_blocking(int fd){
	int flags;
	flags = fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
}

void init_request(request_t* request){
	request->parse_state = METHOD;
	request->action = parse_request_line;
	buffer_t* ib = &request->ib;
	buffer_t* ob = &request->ob;
	init_buffer(ib);
	init_buffer(ob);
}

void init_buffer(buffer_t* buffer){
	buffer->start = (char*)malloc(BUFFER_SIZE);
	buffer->pos = buffer->start;
	buffer->end = buffer->start + BUFFER_SIZE;
	buffer->free = BUFFER_SIZE;
}

conn_t* init_conn(int fd, int epfd){
	conn_t* conn = (conn_t*)malloc(sizeof(conn_t));
	conn->fd = fd;
	conn->epfd = epfd;
	request_t* request = &conn->request;
	init_request(request);
	set_fd_non_blocking(fd);
	return conn;
}

void accept_conn(int listen_fd, int epfd){
	struct epoll_event event;
	int fd = accept(listen_fd, NULL, NULL);
	event.events = EPOLLIN;
	event.data.ptr = init_conn(fd, epfd);
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
}

void change_to_response(conn_t* conn){
	request_t* request = &conn->request;
	struct epoll_event event;
	event.events = EPOLLOUT;
	event.data.ptr = conn;
	epoll_ctl(conn->epfd, EPOLL_CTL_MOD, conn->fd, &event);
	request->action = get_file_info;
}

void close_conn(conn_t* conn){
	request_t* request = &conn->request;
    buffer_t* ib = &request->ib;
	buffer_t* ob = &request->ob;
	close(conn->fd);
	free(ib->start);
	free(ob->start);
	free(conn);
}