#include <stddef.h>
#include <time.h>
#define BUFFER_SIZE 1024
#define CHUNK_SIZE 2048
#define MAXEVENTS 65536
#define OK 0
#define ERROR -1
#define AGAIN -2
#define TIMEOUT 75
#define WORKERS 1
#define DEBUG 1

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

typedef struct buffer{
	char* end;
	char* pos;
	int free;
	struct buffer* next;
} buffer_t;

typedef struct chunk{
	char* start;
	char* pos;
	struct chunk* next;
}chunk_t;

typedef struct{
	chunk_t* first;
	chunk_t* current;
}pool_t;

typedef struct apl_timer{
	struct apl_timer* prev;
	struct apl_timer* next;
	time_t expire_time;
	struct conn* conn;
}apl_timer_t;

typedef struct{
	apl_timer_t* head;
	apl_timer_t* tail;
	int size;
}timers_t;

extern timers_t timers;

typedef int (*header_handler)(struct conn*);

typedef struct request{
	buffer_t* ib;
	buffer_t* ob;
	int parse_state;
	int content_length;   //remain length of response body need to be sent
	int file_fd;
	int (*action)(struct conn*);
	char* uri_start;
	int minor_version;
	char* header_key;
	char* header_value;
	int keep_alive;
	int need_to_copy;   //default = 0
	int status_code;
} request_t;

typedef struct conn{
	int fd;
	int epfd;
	request_t* request;
	pool_t* pool;
	apl_timer_t* timer;
} conn_t;

void ib_realloc(conn_t* conn);
void ob_realloc(conn_t* conn);
void append_out_buffer(conn_t* conn, char* data);

typedef struct node{
	char* key;
	int (*handler)();
}node_t;

typedef struct{
	node_t* table;
	int size;
}dict_t;

dict_t* header_dict;

dict_t* dict_init(int size);
header_handler dict_get(dict_t* dict, char* key);
void dict_add(dict_t* dict, char* key, header_handler);
dict_t* header_dict_init();

chunk_t* chunk_init();
pool_t* pool_init();
void pool_realloc(pool_t* pool);
void* pool_alloc(pool_t* pool, int size);
void* pool_calloc(pool_t* pool, int size);
void pool_free(conn_t* conn);

char* str_cat(conn_t* conn, char* s1, char* s2);

int parse_request_line(conn_t* conn);
int parse_request_header(conn_t* conn);

int handle_response(conn_t* conn);
int handle_request(conn_t* conn);

void conn_register(conn_t* conn);
void conn_unregister(conn_t* conn);
void get_file_info(conn_t* conn);
void append_res_line(conn_t* conn);
void append_res_header(conn_t* conn);
int handle_request_header(conn_t* conn);

int send_file(conn_t* conn);
int header_conn_handler(conn_t* conn);
request_t* request_init(pool_t* pool);
void change_to_response(conn_t* conn);
void request_reset(conn_t* conn);
void change_to_request(conn_t* conn);
int start_listen();
void epoll_add_listen_fd(int listen_fd, int epfd);
void conn_accept(int listen_fd, int epfd);
void conn_expire(conn_t* conn);
void conn_reactive(conn_t* conn);
void conn_clear();
buffer_t* buffer_init(pool_t* pool);
int send_buffer(conn_t* conn);

void timers_add_last(apl_timer_t* timer);
void timers_add_first(apl_timer_t* timer);
void timers_del(apl_timer_t* timer);