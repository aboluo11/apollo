#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdlib.h>
#include "server.h"

dict_t* header_dict = init_header_dict();

void startup(){
	int listen_fd = start_listen();
	if(listen_fd == ERROR){
		printf("listen failed\n");
		return;
	}
	struct epoll_event *events;
	int epfd = epoll_create1(0);
	epoll_add_listen_fd(listen_fd, epfd);
	events = malloc(MAXEVENTS * sizeof(struct epoll_event));
	while(1){
		int n = epoll_wait(epfd, events, MAXEVENTS, 100);
		for(int i=0; i<n; i++){
			int fd = *((int*)(events[i].data.ptr));
			if(fd == listen_fd){
				accept_conn(listen_fd, epfd);
			}else{
				int status;
				conn_t* conn = (conn_t*)(events[i].data.ptr);
				if(events[i].events == EPOLLIN){
					status = handle_request(conn);
				}else if(events[i].events == EPOLLOUT){
					status = handle_response(conn);
				}
				if(status == ERROR) {
					expire_conn(conn);
				}
				else {
					reactive_conn(conn);
				}
			}
		}
		clear_conn();
	}
}

int main(int argc, char* argv[]){
	startup();
}
