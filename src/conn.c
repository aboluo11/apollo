#include "server.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

timers_t timers;

int start_listen(){
    int listen_fd;
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		return ERROR;
	}
	int on = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
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
	int* ptr = malloc(sizeof(int));
	*ptr = listen_fd;
	event.events = EPOLLIN;
	event.data.ptr = ptr;
	epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event);
}

void fd_set_non_blocking(int fd){
	int flags;
	flags = fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
}

buffer_t* buffer_init(pool_t* pool){
	buffer_t* buffer = pool_alloc(pool, sizeof(buffer_t));
	buffer->pos = pool_alloc(pool, BUFFER_SIZE);
	buffer->end = buffer->pos + BUFFER_SIZE;
	buffer->free = BUFFER_SIZE;
	return buffer;
}

request_t* request_init(pool_t* pool){
	request_t* request = pool_alloc(pool, sizeof(request_t));
	request->parse_state = METHOD;
	request->action = parse_request_line;
	request->ib = buffer_init(pool);
	request->ob = buffer_init(pool);
	request->need_to_copy = 0;
	return request;
}

conn_t* conn_init(int fd, int epfd){
	pool_t* pool = pool_init();
	conn_t* conn = malloc(sizeof(conn_t));
	conn->pool = pool;
	conn->fd = fd;
	conn->epfd = epfd;
	fd_set_non_blocking(fd);
	conn->request = request_init(pool);
	return conn;
}

void conn_accept(int listen_fd, int epfd){
	struct epoll_event event;
	int fd = accept(listen_fd, NULL, NULL);
	conn_t* conn = conn_init(fd, epfd);
	event.events = EPOLLIN;
	event.data.ptr = conn;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
	conn_register(conn);
}

void change_to_response(conn_t* conn){
	struct epoll_event event;
	event.events = EPOLLOUT;
	event.data.ptr = conn;
	epoll_ctl(conn->epfd, EPOLL_CTL_MOD, conn->fd, &event);
	get_file_info(conn);
	append_res_line(conn);
	append_res_header(conn);
	conn->request->action = handle_response;
}

void change_to_request(conn_t* conn){
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.ptr = conn;
	epoll_ctl(conn->epfd, EPOLL_CTL_MOD, conn->fd, &event);
	conn->request->action = handle_request;
}

void conn_close(conn_t* conn){
	conn_unregister(conn);
	close(conn->fd);
	epoll_ctl(conn->epfd, EPOLL_CTL_DEL, conn->fd, NULL);
	pool_free(conn);
	free(conn);
}

void timers_add_last(apl_timer_t* timer){
	if(timers.head){
		timer->prev = timers.tail;
		timer->next = NULL;
		timer->prev->next = timer;
		timers.tail = timer;
	}else{
		timer->prev = NULL;
		timer->next = NULL;
		timers.head = timer;
		timers.tail = timer;
	}
}

void timers_add_first(apl_timer_t* timer){
	if(timers.head){
		timer->prev = NULL;
		timer->next = timers.head;
		timers.head = timer;
	}else{
		timers.head = timer;
		timers.tail = timer;
		timer->prev = NULL;
		timer->next = NULL;
	}
}

void timers_del(apl_timer_t* timer){
	if(timer->prev){
		timer->prev->next = timer->next;
	}else{
		timers.head = timer->next;
	}
	if(timers.tail == timer){
		timers.tail = NULL;
	}
}

void conn_register(conn_t* conn){
	apl_timer_t* timer = pool_alloc(conn->pool, sizeof(apl_timer_t));
	timer->expire_time = time(NULL) + TIMEOUT;
	timer->conn = conn;
	conn->timer = timer;
	timers_add_last(timer);
}

void conn_unregister(conn_t* conn){
	timers_del(conn->timer);
}

void conn_clear(){
	time_t now = time(NULL);
	while(timers.head){
		apl_timer_t* head = timers.head;
		if(head->expire_time <= now){
			conn_close(head->conn);
		}else{
			break;
		}
	}
}

void conn_expire(conn_t* conn){
	apl_timer_t* timer = conn->timer;
	timer->expire_time = 0;
	timers_del(timer);
	timers_add_first(timer);
}

void conn_reactive(conn_t* conn){
	apl_timer_t* timer = conn->timer;
	timer->expire_time = time(NULL) + TIMEOUT;
	timers_del(timer);
	timers_add_last(timer);
}