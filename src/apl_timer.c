#include "server.h"

timers_t timers;

void timers_add_last(apl_timer_t* timer){
    if(timers.size == 0){
        timers.head = timer;
        timers.tail = timer;
    }else{
        timer->prev = timers.tail;
        timers.tail->next = timer;
        timers.tail = timer;
    }
    timers.size++;
}

void timers_add_first(apl_timer_t* timer){
    if(timers.size == 0){
        timers.head = timer;
        timers.tail = timer;
    }else{
        timer->next = timers.head;
        timers.head->prev = timer;
        timers.head = timer;
    }
    timers.size++;
}

void timers_del(apl_timer_t* timer){
    if(timers.size == 1){
        timers.head = NULL;
        timers.tail = NULL;
    }else if(timer == timers.head){
        timers.head = timer->next;
    }else if(timer == timers.tail){
        timers.tail = timer->prev;
    }else{
        timer->prev->next = timer->next;
    }
    timer->prev = NULL;
    timer->next = NULL;
    timers.size--;
}