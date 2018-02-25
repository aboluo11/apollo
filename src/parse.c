#include "server.h"

int parse_request_line(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* buffer = request->ib;
    char* p;
    for(p = buffer->pos; p < buffer->end - buffer->free; p++){
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
                request->need_to_copy = 1;
                request->uri_start = p;
                request->parse_state = URI;
                break;
            case URI:
                switch(*p){
                    case ' ':
                        request->need_to_copy = 0;
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
                        goto done;
                    default:
                        return ERROR;
                }
        }
    }
    request->parse_state = HL_LF;
    buffer->pos = p;
    return AGAIN;

    done:
        buffer->pos = p + 1;
        request->action = parse_request_header;
        return parse_request_header(conn);
}

int parse_request_header(conn_t* conn){
    request_t* request = conn->request;
    buffer_t* buffer = request->ib;
    char* p;
    for(p = buffer->pos; p < buffer->end - buffer->free; p++){
        switch(request->parse_state){
            case HL_KEY:
                switch(*p){
                    case ':':
                        request->parse_state = HL_COLON;
                        *p = 0;
                        break;
                    default:
                        break;
                }
                break;
            case HL_COLON:
                switch(*p){
                    case ' ':
                        request->parse_state = HL_SPACE;
                        break;
                    default:
                        return ERROR;
                }
                break;
            case HL_SPACE:
                switch(*p){
                    default:
                        request->parse_state = HL_VALUE;
                        request->header_value = p;
                        break;
                }
                break;
            case HL_VALUE:
                switch(*p){
                    case '\r':
                        request->parse_state = HL_CR;
                        *p = 0;
                        break;
                    default:
                        break;
                }
                break;
            case HL_CR:
                switch(*p){
                    case '\n':
                        request->need_to_copy = 0;
                        request->parse_state = HL_LF;
                        if(handle_request_header(conn) == ERROR){
                            return ERROR;
                        }
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
                        request->need_to_copy = 1;
                        request->parse_state = HL_KEY;
                        request->header_key = p;
                        break;
                }
                break;
            case HL_END_CR:
                switch(*p){
                    case '\n':
                        goto done;
                    default:
                        return ERROR;
                }
        }
    }
    buffer->pos = p;
    return AGAIN;

    done:
        buffer->pos = p + 1;
        request->parse_state = HL_DONE;
        return OK;
}