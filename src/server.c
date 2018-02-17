#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdlib.h>
#include "server.h"

void startup(){
	int listen_fd = start_listen();
	if(listen_fd == ERROR){
		printf("listen failed\n");
		return;
	}
	struct epoll_event *events;
	int epfd = epoll_create1(0);
	epoll_add_listen_fd(listen_fd, epfd);
	events = (struct epoll_event*)malloc(MAXEVENTS * sizeof(struct epoll_event));
	while(1){
		int n = epoll_wait(epfd, events, MAXEVENTS, -1);
		for(int i=0; i<n; i++){
			int fd = *((int*)(events[i].data.ptr));
			if(fd == listen_fd){
				accept_conn(listen_fd, epfd);
			}else{
				conn_t* conn = (conn_t*)(events[i].data.ptr);
				if(events[i].events == EPOLLIN){
					int status = handle_request(conn);
					if(status == ERROR){
						close_conn(conn);
					}
				}else if(events[i].events == EPOLLOUT){
					int status = handle_response(conn);
					if(status == OK || status == ERROR){
						close_conn(conn);
					}
				}
			}
		}
	}
}

int main(int argc, char* argv[]){
	startup();
}
