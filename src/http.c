#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "../include/http.h"
#include "../include/log.h"

static void parse_http_req_method(char** req_str, struct http_request_t* req) {
    if (req_str == NULL || *req_str == NULL || req == NULL) {
        log(ERROR, "Invalid function arguments");
        return;
    }

    size_t get_len = strlen(STR_GET);
    if (strncmp(*req_str, STR_GET, get_len) == 0) {
        req->method = GET;
        *req_str += get_len + 1; // len of GET + single space
        return;
    }
    size_t head_len = strlen(STR_HEAD);
    if (strncmp(*req_str, STR_HEAD, head_len) == 0) {
        req->method = HEAD;
        *req_str += head_len + 1; //len of HEAD + single space
        return;
    }
    req->method = METHOD_UNDEFINED;
}

static void url_decode(char* url) {
    char url_repl_buf[4096 * 4];
    char* url_repl = strcpy(url_repl_buf, url);
    if (url_repl == NULL) {
        log(ERROR, "Unable to copy URL to buffer");
        return;
    }
    char* cursor = strchr(url_repl, '%');
    while (cursor != NULL) {
        int hex_chr = 0;
        sscanf(cursor + 1, "%x", &hex_chr);
        char chr = (char)hex_chr;
        *cursor = chr;
        strcpy(cursor + 1, cursor + 3);
        cursor = strchr(cursor, '%');
    }
    strcpy(url, url_repl);
}

static void parse_http_req_uri(char** req_str, struct http_request_t* req) {
    if (req_str == NULL || *req_str == NULL || req == NULL) {
        log(ERROR, "Invalid function arguments");
        return;
    }

    req->URI = *req_str;
    *req_str = strchr(*req_str, ' ');
    if (*req_str == NULL) {
        req->URI = NULL;
        return;
    }
    **req_str = '\0';
    *req_str += 1;
    url_decode(req->URI);

    char* query_start = strrchr(req->URI, '?');
    if (query_start != NULL) {
        *query_start = '\0';
    }
}

static void parse_http_req_proto_ver(char** req_str, struct http_request_t* req) {
    if (req_str == NULL || *req_str == NULL || req == NULL) {
        log(ERROR, "Invalid function arguments");
        return;
    }

    size_t http_v1_0_len = strlen(STR_HTTPv1_0);
    if (strncmp(*req_str, STR_HTTPv1_0, http_v1_0_len) == 0) {
        req->http_version=HTTPv1_0;
        *req_str += http_v1_0_len + 2; // len of HTTP/1.0 + \r\n
        return;
    }
    size_t http_v1_1_len = strlen(STR_HTTPv1_0);
    if (strncmp(*req_str, STR_HTTPv1_1, http_v1_1_len) == 0) {
        req->http_version=HTTPv1_1;
        *req_str += http_v1_1_len + 2; // len of HTTP/1.1 + \r\n
        return;
    }
    req->http_version = VERSION_UNDEFINED;
}

static void parse_http_req_headers(char** req_str, struct http_request_t* req) {
    if (req_str == NULL || *req_str == NULL || req == NULL) {
        log(ERROR, "Invalid function arguments");
        return;
    }

    const char* delim = "\r\n";
    const int delim_len = 2;
    char** cursor = req_str;
    size_t header_len = 0;
    size_t headers_count = 0;

    while(1) {
	char* prev_cursor_val = *cursor;
        *cursor = strstr(*cursor, delim);
        header_len = *cursor - prev_cursor_val;
        log(DEBUG, "header_len: %d", header_len);
        if(header_len == 0) {
            *cursor += delim_len;
            req->headers_count = headers_count;
            break;
        }
        if (*cursor == NULL) {
            log(ERROR, "Can't parse headers: empty line does not reached");
            req->headers_count = 0;
            return;
        }
        if (headers_count > req->headers_count) {
            log(ERROR, "Not enough memory allocated for headers");
            req->headers_count = 0;
            return;
        }
        req->headers[headers_count].text = prev_cursor_val;
        req->headers[headers_count].len = header_len + delim_len; //CRLF is a part of a header too
        *cursor += delim_len;
        headers_count++;
    }
}

/*static void parse_http_req_body(char** req_str, struct http_request_t* req) {
    if (req_str == NULL || *req_str == NULL || req == NULL || req->headers == NULL) {
        log(ERROR, "Invalid function arguments");
        return;
    }
    //TODO develop?
}*/

char* request_method_t_to_string(enum request_method_t method) {
    switch (method) {
        case HEAD: {
            return STR_HEAD;
        }
        case GET: {
            return STR_GET;
        }
        default: {
            return "METHOD_UNDEFINED\0";
        }
    }
}

char* http_version_t_to_string(enum http_version_t version) {
    switch (version) {
        case HTTPv1_0: {
            return STR_HTTPv1_0;
        }
        case HTTPv1_1: {
            return STR_HTTPv1_1;
        }
        default: {
            return "VERSION_UNDEFINED";
        }
    }
}

char* http_state_t_to_string(enum http_state_t state) {
    switch (state) {
        case OK: {
            return STR_200_OK;
        }
        case BAD_REQUEST: {
            return STR_400_BAD_REQUEST;
        }
        case FORBIDDEN: {
            return STR_403_FORBIDDEN;
        }
        case NOT_FOUND: {
            return STR_404_NOT_FOUND;
        }
        case METHOD_NOT_ALLOWED: {
            return STR_405_METHOD_NOT_ALLOWED;
        }
        case INTERNAL_SERVER_ERROR: {
            return STR_500_INTERNAL_SERVER_ERROR;
        }
        default: {
            return "STATE_UNDEFINED\0";
        }
    }
}

enum http_state_t parse_http_request(char* req_str, struct http_request_t* req) {
    if (req_str == NULL || req ==NULL) {
        log(ERROR, "Invalid function arguments");
        return INTERNAL_SERVER_ERROR;
    }

    char* cursor = req_str;
    while (*cursor == '\r' || *cursor == '\n') {
        cursor += 1;
    }
    if (*cursor == '\0') {
        log(INFO, "Skipped empty req_str while parsing request");
        return BAD_REQUEST;
    }

    parse_http_req_method(&cursor, req);
    log(DEBUG, "HTTP request method parsed: %s", request_method_t_to_string(req->method));
    if (req->method == METHOD_UNDEFINED) {
        return METHOD_NOT_ALLOWED;
    }

    parse_http_req_uri(&cursor, req);
    log(DEBUG, "HTTP request URI parsed: %s", req->URI);
    if (req->URI == NULL) {
        log(DEBUG, "parse_http_request returning BAD_REQUEST, URI==NULL");
        return BAD_REQUEST;
    }

    parse_http_req_proto_ver(&cursor, req);
    log(DEBUG, "HTTP request protocol version parsed: %s", http_version_t_to_string(req->http_version));
    if (req->http_version == VERSION_UNDEFINED) {
        log(DEBUG, "parse_http_request returning BAD_REQUEST, HTTP_VERSION==VERSION_UNDEFINED");
        return BAD_REQUEST;
    }

    if (req->headers_count > 0) {
        parse_http_req_headers(&cursor, req);
        log(DEBUG, "HTTP headers parsed:");
#ifdef DEBUG_MODE
        for (size_t i = 0; i < req->headers_count; i++) {
            log(DEBUG, "%.*s", req->headers[i].len - 2, req->headers[i].text);
        }
#endif
        if (req->headers_count == 0) {
            log(DEBUG, "parse_http_request returning INTERNAL_SERVER_ERROR, unable to parse headers");
            return INTERNAL_SERVER_ERROR;
        }
    } else {
        log(DEBUG, "There are no headers to parse in request");
    }

    return OK;
}

int build_date_header(struct http_header_t* header) {
    if (header == NULL) {
        log(ERROR, "Invalid function arguments");
        return -1;
    }

    time_t raw_time; //timestamp
    if (time(&raw_time) < 0) {
        log(ERROR, "Unable to get calendar time");
        return -1;
    }

    struct tm* time_info = localtime(&raw_time); //datetime
    if (time_info == NULL) {
        log(ERROR, "Unable to parse raw_time into time_info");
        return -1;
    }

    size_t header_len = strftime(header->text, HTTP_HEADER_DEFAULT_BUFFER_SIZE, "Date: %a, %d %b %Y %X %Z\r\n", time_info);
    if (header_len == 0) {
        log(ERROR, "Can not build Date header: buffer is too small");
        return -1;
    }

    header->len = strlen(header->text);
    return 0;
}

static int build_content_length_header(struct http_header_t* header, int64_t content_len) {
    if (header == NULL || header->text == NULL) {
        log(ERROR, "Invalid function arguments");
        return -1;
    }
    strcpy(header->text, STR_CONTENT_LENGTH_HEADER);
    sprintf(header->text + strlen(header->text), "%lu\r\n", content_len);
    header->len = strlen(header->text);
    return 0;
}

static int build_content_type_header(struct http_header_t* header, enum mime_t mime_type) {
    if (header == NULL || header->text == NULL) {
        log(ERROR, "Invalid function arguments");
        return -1;
    }
    strcpy(header->text, STR_CONTENT_TYPE_HEADER);
    strcat(header->text, mime_type_to_str(mime_type));
    strcat(header->text, "\r\n\0");
    header->len = strlen(header->text);
    return 0;
}

enum http_state_t build_http_response(struct http_request_t* req, struct http_response_t* resp) {
    const size_t headers_count = 5;
    if (req == NULL || resp == NULL || resp->headers == NULL || resp->headers_count < headers_count) {
        log(ERROR, "Invalid function arguments");
        return INTERNAL_SERVER_ERROR;
    }
    size_t header_idx = 0;
    if (req->http_version == HTTPv1_1) {
        resp->headers[header_idx] = (struct http_header_t) {
                STR_CONNECTION_KEEP_ALIVE_HEADER,
                strlen(STR_CONNECTION_KEEP_ALIVE_HEADER)
        };
    } else {
        resp->headers[header_idx] = (struct http_header_t) {
            STR_CONNECTION_CLOSE_HEADER,
            strlen(STR_CONNECTION_CLOSE_HEADER)
        };
    }
    header_idx++;

    int build_result = build_date_header(&resp->headers[header_idx]);
    if (build_result < 0) {
        log(ERROR, "Unable to build Date header");
        resp->headers[header_idx] = (struct http_header_t){
                STR_DEFAULT_DATE_HEADER,
                strlen(STR_DEFAULT_DATE_HEADER)
        };
    }
    header_idx++;

    bool should_get_fd = req->method==GET;
    enum file_state_t inspect_result = inspect_file(req->URI, &resp->file_to_send, should_get_fd);
    switch (inspect_result) {
        case FILE_STATE_OK: {
            log(DEBUG, "File inspection successfully finished");
            break;
        }
        case FILE_STATE_NOT_FOUND: {
            log(DEBUG, "File inspection finished: file not found");
            return NOT_FOUND;
        }
        case FILE_STATE_FORBIDDEN: {
            log(DEBUG, "File inspection finished: access forbidden");
            return FORBIDDEN;
        }
        case FILE_STATE_INTERNAL_ERROR: {
            log(ERROR, "File inspection finished цшер штеуктфд уккщк");
            return INTERNAL_SERVER_ERROR;
        }
        default: {
            log(ERROR, "Unknown return code: %d", inspect_result);
            return INTERNAL_SERVER_ERROR;
        }
    }
    log(DEBUG, "File_to_send: fd: %d, len: %d, mime-type: %s",
            resp->file_to_send.fd, resp->file_to_send.mime_type, mime_type_to_str(resp->file_to_send.mime_type));

    build_result = build_content_length_header(&resp->headers[header_idx], resp->file_to_send.len);
    if (build_result < 0) {
        log(WARNING, "Unable to build Content-Length header");
    } else {
        header_idx++;
    }
    log(DEBUG, "Content-Length header built");

    build_result = build_content_type_header(&resp->headers[header_idx], resp->file_to_send.mime_type);
    if (build_result) {
        log(WARNING, "Unable to build Content-Type header");
    } else {
        header_idx++;
    }
    log(DEBUG, "Content-Type header built");

    resp->headers[header_idx] = (struct http_header_t){
            STR_SERVER_HEADER,
            strlen(STR_SERVER_HEADER)
    };
    header_idx++;

    resp->code = OK;
    resp->http_version = req->http_version;
    resp->headers_count = headers_count;
    return OK;
}
