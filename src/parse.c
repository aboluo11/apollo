#include "server.h"

int parse_request_line(conn_t* conn){
    request_t* request = &conn->request;
    buffer_t* buffer = &request->ib;
    for(char* p = buffer->pos; p < buffer->end - buffer->free; p++, buffer->pos++){
        switch(request->parse_state){
            case METHOD:
                switch(*p){
                    case 'G':
                    case 'E':
                    case 'T':
                        break;
                    case ' ':
                        request->parse_state = SPACE_BEFORE_URI;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case SPACE_BEFORE_URI:
                request->uri_start = p;
                request->parse_state = URI;
                break;
            case URI:
                switch(*p){
                    case ' ':
                        request->uri_end = p;
                        request->parse_state = SPACE_BEFORE_VERSION;
                        *p = '\0';
                        break;
                }
                break;
            case SPACE_BEFORE_VERSION:
                switch(*p){
                    case 'H':
                        request->parse_state = VERSION_H;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case VERSION_H:
                switch(*p){
                    case 'T':
                        request->parse_state = VERSION_HT;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case VERSION_HT:
                switch(*p){
                    case 'T':
                        request->parse_state = VERSION_HTT;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case VERSION_HTT:
                switch(*p){
                    case 'P':
                        request->parse_state = VERSION_HTTP;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case VERSION_HTTP:
                switch(*p){
                    case '/':
                        request->parse_state = SLASH_BEFORE_MAJOR_VERSION;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case SLASH_BEFORE_MAJOR_VERSION:
                switch(*p){
                    case '1':
                        request->parse_state = MAJOR_VERSION;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case MAJOR_VERSION:
                switch(*p){
                    case '.':
                        request->parse_state = DOT;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case DOT:
                switch(*p){
                    case '1':
                        request->parse_state = MINOR_VERSION;
                        request->minor_version = 1;
                        break;
                    case '0':
                        request->parse_state = MINOR_VERSION;
                        request->minor_version = 0;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case MINOR_VERSION:
                switch(*p){
                    case '\r':
                        request->parse_state = RL_ALMOST_DONE;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case RL_ALMOST_DONE:
                switch(*p){
                    case '\n':
                        request->parse_state = HL_NORMAL;
                        request->action = parse_request_header;
                        buffer->pos++;
                        return OK;
                    default:
                        return ERROR;
                }
        }
    }
    return AGAIN;
}

int parse_request_header(conn_t* conn){
    request_t* request = &conn->request;
    buffer_t* buffer = &request->ib;
    for(char* p = buffer->pos; p < buffer->end - buffer->free; p++, buffer->pos++){
        switch(request->parse_state){
            case HL_NORMAL:
                switch(*p){
                    case '\r':
                        request->parse_state = HL_CR;
                        break;
                    default:
                        break;
                }
                break;
            case HL_CR:
                switch(*p){
                    case '\n':
                        request->parse_state = HL_LF;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case HL_LF:
                switch(*p){
                    case '\r':
                        request->parse_state = HL_END_CR;
                        break;
                    default:
                        request->parse_state = HL_NORMAL;
                        break;
                }
                break;
            case HL_END_CR:
                switch(*p){
                    case '\n':
                        request->parse_state = HL_DONE;
                        request->action = NULL;
                        buffer->pos++;
                        return OK;
                    default:
                        return ERROR;
                }
        }
    }
    return AGAIN;
}