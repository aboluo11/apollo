#include <stdlib.h>
#include <string.h>

char* str_cat(char* s1, char* s2){
    char* res = (char*)calloc(1, strlen(s1)+strlen(s2)+1);
    strcat(res, s1);
    strcat(res, s2);
    return res;
}