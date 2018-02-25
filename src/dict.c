#include <string.h>
#include <stdlib.h>
#include "server.h"

static int compute_hash(char* str, int size){
    unsigned int hash = 0;
    unsigned int seed = 131;
    while(*str){
        hash = hash * seed + (*str);
        str++;
    }
    return hash & (size - 1);
}

dict_t* dict_init(int size){
    dict_t* dict = malloc(sizeof(dict_t));
    dict->size = size;
    dict->table = calloc(size, sizeof(node_t));
    return dict;
}

header_handler dict_get(dict_t* dict, char* key){
    int hash = compute_hash(key, dict->size);
    node_t* end = dict->table + dict->size;
    node_t* node = dict->table + hash;
    while(node->key){
        if(strcmp(key, node->key) == 0){
            return node->handler;
        }else{
            if(node + 1 < end){
                node++;
            }else{
                node = dict->table;
            }
        }
    }
    return NULL;
}

void dict_add(dict_t* dict, char* key, header_handler handler){
    int hash = compute_hash(key, dict->size);
    node_t* end = dict->table + dict->size;
    node_t* node = dict->table + hash;
    while(node->key){
        if(node + 1 < end){
            node++;
        }else{
            node = dict->table;
        }
    }
    node->key = key;
    node->handler = handler;
}

dict_t* header_dict_init(){
    dict_t* dict = dict_init(256);
    dict_add(dict, "Connection", header_conn_handler);
    return dict;
}