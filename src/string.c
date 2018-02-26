#include <stdlib.h>
#include <string.h>
#include "server.h"

char* str_cat(conn_t* conn, char* s1, char* s2){
    char* res = pool_calloc(conn->request->pool, strlen(s1) + strlen(s2) + 1);
    strcat(res, s1);
    strcat(res, s2);
    return res;
}

int stoi(char* str){
    char* end = str + strlen(str);
    int res = (int)strtol(str, &end, 10);
    return res;
}