#include <stddef.h>
#define BUFFER_SIZE 1024
#define CHUNK_SIZE 2048
#define MAXEVENTS 65536
#define OK 0
#define ERROR -1
#define AGAIN -2
#define TIMEOUT 75

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
	HL_KEY,
	HL_VALUE,
	HL_COLON,
	HL_SPACE,
	HL_CR,
	HL_LF,
	HL_END_CR,
	HL_DONE
};

struct conn;

typedef int (*)(request_t*) header_handler;

typedef struct{
	char* key;
	int (*handler)();
}entry_t;

typedef struct node{
	entry_t entry;
	struct node* next;
}node_t;

typedef struct{
	node_t* table;
	int size;
}dict_t;

typedef struct chunk{
	char* start;
	char* pos;
	struct chunk* next;
}chunk_t;

typedef struct{
	chunk_t* first;
	chunk_t* current;
}pool_t;

typedef struct buffer{
	char* end;
	char* pos;
	int free;
	buffer_t* next;
} buffer_t;

typedef struct request{
	buffer_t* ib;
	buffer_t* ob;
	int parse_state;
	int content_length;   //remain length of response body need to be sent
	int file_fd;          //default -1
	int (*action)(struct conn*);
	char* uri_start;
	int minor_version;
	char* header_key;
	char* header_value;
	int keep_alive;
	int need_to_copy;   //default = 0
} request_t;

typedef struct conn{
	int fd;
	int epfd;
	request_t* request;
	pool_t* pool;
	time_t expire_time;
	int heap_index;
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
int handle_request_header(conn_t* conn);
int (*)() get_dict(dict_t* dict, char* key);