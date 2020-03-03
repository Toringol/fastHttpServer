#ifndef HIGHLOADSERVER_HTTP_H
#define HIGHLOADSERVER_HTTP_H

#include "config.h"
#include "file_system.h"

enum request_method_t {
    METHOD_UNDEFINED,
    HEAD,
    GET
};
#define STR_HEAD "HEAD\0"
#define STR_GET "GET\0"

enum http_version_t {
    VERSION_UNDEFINED,
    HTTPv1_0,
    HTTPv1_1
};
#define STR_HTTPv1_0 "HTTP/1.0\0"
#define STR_HTTPv1_1 "HTTP/1.1\0"

enum http_state_t {
    STATE_UNDEFINED = 0,
    OK = 200,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500
};
#define STR_200_OK "200 OK\0"
#define STR_400_BAD_REQUEST "400 Bad Request\0"
#define STR_403_FORBIDDEN "403 Forbidden\0"
#define STR_404_NOT_FOUND "404 Not Found\0"
#define STR_405_METHOD_NOT_ALLOWED "405 Method Not Allowed\0"
#define STR_500_INTERNAL_SERVER_ERROR "500 Internal Server Error\0"

#define STR_CONNECTION_KEEP_ALIVE_HEADER "Connection: keep-alive\r\n\0"
#define STR_CONNECTION_CLOSE_HEADER "Connection: close\r\n\0"
#define STR_DEFAULT_DATE_HEADER "Date: Thu, 1 Jan 1970 00:00:00 GMT\r\n\0"
#define STR_SERVER_HEADER "Server: "APP_NAME"/"VERSION"\r\n\0"
#define STR_CONTENT_TYPE_HEADER "Content-Type: \0"
#define STR_CONTENT_LENGTH_HEADER "Content-Length: \0"
#define STR_CONTENT_LENGTH_ZERO_HEADER "Content-Length: 0\r\n\0"

struct http_header_t {
    char* text;
    size_t len;
};
#define HTTP_HEADER_DEFAULT_BUFFER_SIZE 64
#define HTTP_HEADER_INITIALIZER {NULL, 0}

int build_date_header(struct http_header_t* header);

struct http_body_t {
    char* text;
    size_t len;
};
#define HTTP_BODY_INITIALIZER {NULL, 0}

struct http_request_t {
    enum request_method_t method;
    char* URI;
    enum http_version_t http_version;
    struct http_header_t* headers;
    size_t headers_count;
}; //TODO make constructor and destructor?
#define HTTP_REQUEST_INITIALIZER {METHOD_UNDEFINED, NULL, VERSION_UNDEFINED, NULL, 0}

enum http_state_t parse_http_request(char* req_str, struct http_request_t* req);

struct http_response_t {
    enum http_state_t code;
    enum http_version_t http_version;
    struct http_header_t* headers;
    size_t headers_count;
    struct file_t file_to_send;
};
#define HTTP_RESPONSE_INITIALIZER {STATE_UNDEFINED, VERSION_UNDEFINED, NULL, 0, FILE_INITIALIZER}

enum http_state_t build_http_response(struct http_request_t* req, struct http_response_t* resp);

char* request_method_t_to_string(enum request_method_t method);
char* http_version_t_to_string(enum http_version_t version);
char* http_state_t_to_string(enum http_state_t state);

#endif //HIGHLOADSERVER_HTTP_H
