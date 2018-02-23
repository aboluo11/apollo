#include "server.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

conn_t* heap[MAXEVENTS + 1];
int heap_size;

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
	int* ptr = malloc(sizeof(int));
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

buffer_t* init_buffer(pool_t* pool){
	buffer_t* buffer = alloc_pool(pool, sizeof(pool_t));
	buffer->pos = alloc_pool(pool_t, BUFFER_SIZE);
	buffer->end = buffer->start + BUFFER_SIZE;
	buffer->free = BUFFER_SIZE;
	return buffer;
}

request_t* init_request(pool_t* pool){
	request_t* request = alloc_pool(pool, sizeof(request_t));
	request->parse_state = METHOD;
	request->action = parse_request_line;
	request->ib = init_buffer(pool);
	request->ob = init_buffer(pool);
	request->need_to_copy = 0;
	return request;
}

conn_t* init_conn(int fd, int epfd, pool_t* pool){
	pool_t* pool = init_pool();
	conn_t* conn = malloc(sizeof(conn_t));
	conn->pool = pool;
	conn->fd = fd;
	conn->epfd = epfd;
	set_fd_non_blocking(fd);
	conn->request = init_request(pool);
	return conn;
}

void accept_conn(int listen_fd, int epfd){
	struct epoll_event event;
	int fd = accept(listen_fd, NULL, NULL);
	conn_t* conn = init_conn(fd, epfd);
	event.events = EPOLLIN;
	event.data.ptr = conn;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
	register_conn(conn);
}

void change_to_response(conn_t* conn){
	struct epoll_event event;
	event.events = EPOLLOUT;
	event.data.ptr = conn;
	epoll_ctl(conn->epfd, EPOLL_CTL_MOD, conn->fd, &event);
	get_file_info();
	append_res_line(conn);
	append_res_header(conn);
	request->action = handle_response;
}

void change_to_request(conn_t* conn){
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.ptr = conn;
	epoll_ctl(conn->epfd, EPOLL_CTL_MOD, conn->fd, &event);
	request->action = handle_request;
}

void free_conn(conn_t* conn){
	chunk_t* chunk = conn->pool->first;
	chunk_t* next;
	while(chunk){
		next = chunk->next;
		free(chunk);
		chunk = next;
	}
}

void close_conn(conn_t* conn){
	free_pool(conn);
	unregister_conn(conn);
	close(conn->fd);
	epoll_ctl(conn->epfd, EPOLL_CTL_DEL, conn->fd, NULL);
	free(conn);
}

void register_conn(conn_t* conn){
	conn->expire_time = time(NULL) + TIMEOUT;
	heap[heap_size + 1] = conn;
	heap_size++;
	conn->heap_index = heap_size;
}

void unregister_conn(conn_t* conn){
	int index = conn->heap_index;
	heap[index] = heap[heap_size];
	heap_size--;
	conn->heap_index = index;
	bubble_down_heap(conn);
}

void clear_conn(){
	time_t now = time(NULL);
	while(heap_size){
		if(heap[1]->expire_time >= now){
			close_conn(heap[1]);
		}else{
			break;
		}
	}
}

void expire_conn(conn_t* conn){
	conn->expire_time = 0;
	bubble_up_heap(conn);
}

void reactive_conn(conn_t* conn){
	conn->expire_time = time(NULL) + TIMEOUT;
	bubble_down_heap(conn);
}

void swap_heap(int index1, int index2){
	conn_t* c1 = heap[index1];
	conn_t* c2 = heap[index2];
	conn_t* temp1 = heap[index1];
	heap[index1] = heap[index2];
	heap[index2] = temp1;
	int temp2 = c1->heap_index;
	c1->heap_index = c2->heap_index;
	c2->heap_index = temp2;
}

void bubble_up_heap(conn_t* conn){
	int index = conn->heap_index;
	while(index >= 1){
		int par_index = (index - 1) / 2;
		if(heap[index]->expire_time < heap[par_index]->expire_time){
			swap_heap(index, par_index);
			index = par_index;
		}else{
			break;
		}
	}
}

void bubble_down_heap(conn_t* conn){
	int index = conn->index;
	if(index * 2 > heap_size) return;
	if(index * 2 == heap_size){
		if(heap[index]->expire_time > heap[2 * index]->expire_time){
			swap_heap(index, 2 * index);
		}
		return;
	}
	if(heap[index]->expire_time > heap[2 * index]->expire_time){
		swap_heap(index, 2 * index);
		bubble_down_heap(conn);
		bubble_down_heap(heap[index]);
	}else if(heap[index]->expire_time > heap[2 * index + 1]->expire_time){
		swap_heap(index, 2 * index + 1);
		bubble_down_heap(conn);
	}
}