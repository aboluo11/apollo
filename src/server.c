#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "server.h"

config_t config;

dict_t* header_dict_init(){
    dict_t* dict = dict_init(256);
    dict_add(dict, "Connection", header_conn_handler);
    return dict;
}

void startup(){
	signal(SIGPIPE, SIG_IGN);
	header_dict = header_dict_init();
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
		int n = epoll_wait(epfd, events, MAXEVENTS, 20);
		for(int i=0; i<n; i++){
			int fd = *((int*)(events[i].data.ptr));
			if(fd == listen_fd){
				conn_accept(listen_fd, epfd);
			}else{
				int status;
				conn_t* conn = (conn_t*)(events[i].data.ptr);
				if(events[i].events == EPOLLIN){
					status = handle_request(conn);
				}else if(events[i].events == EPOLLOUT){
					status = handle_response(conn);
				}
				if(status == ERROR) {
					conn_expire(conn);
				}
				else {
					conn_reactive(conn);
				}
			}
		}
		conn_clear();
	}
}

int config_init(char* directive, char* value){
	if(strcmp(directive, "-p") == 0){
		config.port = stoi(value);
	}else if(strcmp(directive, "-r") == 0){
		config.root = value;
	}else{
		return ERROR;
	}
	return OK;
}

void config_init_error(){
	printf("usage: apollo -p port -r root_path\n");
	exit(0);
}

int valid_config(){
	if(config.port && config.root){
		return OK;
	}else{
		return ERROR;
	}
}

int main(int argc, char* argv[]){
	int i = 1;
	while(i < argc && i + 1 < argc){
		char* directive = argv[i];
		char* value = argv[i + 1];
		int status = config_init(directive, value);
		if(status == ERROR){
			config_init_error();
		}else{
			i += 2;
		}
	}
	if(valid_config() == ERROR){
		config_init_error();
	}

	if(DEBUG){
		goto debug;
	}
	for(int i = 0; i < WORKERS; i++){
		if(fork() == 0){
			startup();
		}
	}
	int wstatus;
	wait(&wstatus);

	debug:
		startup();
}
