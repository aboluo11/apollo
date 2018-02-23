#include <string.h>

int compute_hash(char* str, int size){
    unsigned int hash = 0;
    unsigned int seed = 131;
    while(*str){
        hash = hash * seed + (*str);
        str++;
    }
    return hash & (size - 1);
}

void set_entry(entry_t* entry, char* key, void (*handler)()){
    entry->key = key;
    entry->handler = handler;
}

dict_t* init_dict(int size){
    dict_t* dict = malloc(sizeof(dict_t));
    dict->size = size;
    dict->table = calloc(size, sizeof(node_t));
    return dict;
}

int (*)() get_dict(dict_t* dict, char* key){
    int hash = compute_hash(key);
    node_t* node = &dict->table[hash];
    while(node->entry.key){
        if(strcmp(key, node->entry.key) == 0){
            return node->entry.handler;
        }
        node = node->next;
    }
    return NULL;
}

void add_dict(dict* dict, char* key, void (*handler)()){
    int hash = compute_hash(key);
    node_t* node = &dict->table[hash];
    while(node->entry.key){
        node = node->next;
    }
    set_entry(&node.entry, key, handler);
}

dict_t* init_header_dict(){
    dict_t* dict = init_dict(256);
    add_dict(dict, "Connection", header_conn_handler);
    return dict;
}