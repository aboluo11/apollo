#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stddef.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include "../src/server.h"

void dict_test(){
    int f(conn_t* conn){}
    int g(conn_t* conn){}
    dict_t* dict = dict_init(256);
    dict_add(dict, "aaa", f);
    dict_add(dict, "bbb", g);
    assert(dict_get(dict, "aaa") == f);
    assert(dict_get(dict, "bbb") == g);
}

void timers_print(timers_t* timers){
    apl_timer_t* timer = timers->head;
    while(timer){
        printf("%d\n", timer->expire_time);
        timer = timer->next;
    }
}

void timer_test(){
    apl_timer_t* a = calloc(1, sizeof(apl_timer_t));
    apl_timer_t* b = calloc(1, sizeof(apl_timer_t));
    apl_timer_t* c = calloc(1, sizeof(apl_timer_t));
    a->expire_time = 1;
    b->expire_time = 2;
    c->expire_time = 3;
    timers_add_last(a);
    timers_add_first(b);
    timers_add_last(c);
    timers_del(b);
    timers_print(&timers);
}

int main(){
    dict_test();
    timer_test();
}