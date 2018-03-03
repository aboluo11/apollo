#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "../src/server.h"

char* req_str = 
"GET / HTTP/1.1\r\nHost: 120.25.229.44:8080\r\nConnection: close\r\nPragma: no-cache\r\nCache-Control: no-cache\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.132 Safari/537.36\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.9,zh-CN;q=0.8,zh;q=0.7\r\n\r\n";

int startup(){
	int conn_fd;
	struct sockaddr_in server_addr = {0};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8080);
	server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	conn_fd = socket(AF_INET, SOCK_STREAM, 0);
	connect(conn_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	return conn_fd;
}

int main(){
	int conn_fd = startup();
	assert(conn_fd > 0);
	int len = strlen(req_str);
	for(int i = 0; i < len; i++){
		send(conn_fd, req_str + i, 1, 0);
		usleep(10 * 1000);
	}
	while(1){
		char ch;
		int len = recv(conn_fd, &ch, 1, 0);
		if(len == 0){
			break;
		}
		write(1, &ch, 1);
	}
}