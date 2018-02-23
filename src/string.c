#include <stdlib.h>
#include <string.h>

char* str_cat(conn_t* conn, char* s1, char* s2){
    char* res = calloc_pool(conn->pool, strlen(s1) + strlen(s2) + 1);
    strcat(res, s1);
    strcat(res, s2);
    return res;
}