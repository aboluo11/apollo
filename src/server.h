#define BUFFER_SIZE 1024
#define MAXEVENTS 65536
#include <stddef.h>

enum PARSE_STATE {
	METHOD,
	SPACE_BEFORE_URI,
	URI,
	SPACE_BEFORE_VERSION,
	VERSION_H,
	VERSION_HT,
	VERSION_HTT,
	VERSION_HTTP,
	SLASH_BEFORE_MAJOR_VERSION,
	MAJOR_VERSION,
	DOT,
	MINOR_VERSION,
	RL_ALMOST_DONE,
	RL_DONE,
	HL_NORMAL,
	HL_CR,
	HL_LF,
	HL_END_CR,
	HL_DONE
};

enum STATUS{
	OK,
	ERROR,
	AGAIN
};

struct conn;

typedef struct buffer{
	char* start;   //start of buffer
	char* end;	   //end of buffer, exclude
	char* pos;     //exclude
	int free;
} buffer_t;

typedef struct request{
	buffer_t ib;
	buffer_t ob;
	int parse_state;
	int content_length;   //remain length of response body need to be sent
	int file_fd;
	int (*action)(struct conn* conn);
	char* uri_start;
	char* uri_end;
	int minor_version;
} request_t;

typedef struct conn{
	int fd;
	int epfd;
	request_t request;
} conn_t;

int start_listen();
void epoll_add_listen_fd(int listen_fd, int epfd);
void startup();
int parse_request_line(conn_t* conn);
int parse_request_header(conn_t* conn);
void set_fd_non_blocking(int fd);
void init_request(request_t* request);
void init_buffer(buffer_t* buffer);
conn_t* init_conn(int fd, int epfd);
void accept_conn(int listen_fd, int epfd);
void change_to_response(conn_t* conn);
void close_conn(conn_t* conn);
void realloc_ib(request_t* request);
void ensure_ib_available(request_t* request);
void realloc_ob(buffer_t* buffer);
int append_out_buffer(buffer_t* buffer, char* data);
int read_in_stream(conn_t* conn);
int handle_request(conn_t* conn);
int append_res_line(conn_t* conn);
int append_res_header(conn_t* conn);
int send_buffer(conn_t* conn);
int send_file(conn_t* conn);
int get_file_info(conn_t* conn);
int handle_response(conn_t* conn);
char* str_cat(char* s1, char* s2);