#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <arpa/inet.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <zconf.h>
#include <event.h>

#include "../include/server.h"
#include "../include/log.h"
#include "../include/http.h"

static void socket_close_cb(struct evbuffer *buffer, const struct evbuffer_cb_info *info, void *arg) {
    if (evbuffer_get_length(buffer) == 0) {
        log(DEBUG, "Freeing the bufferevent");
        struct bufferevent* bev = (struct bufferevent*)arg;
        struct evbuffer* output = bufferevent_get_output(bev);
        if (evbuffer_get_length(output) == 0) {
            int fd = bufferevent_getfd(bev);
            log(WARNING, "Closing file descriptor %d", fd);
            bufferevent_free(bev);
        }
    }
}

static void respond(struct bufferevent* bev, struct evbuffer* output, struct http_response_t* resp) {
    log(DEBUG, "HTTP response:");
    log(DEBUG, "%s %s", http_version_t_to_string(resp->http_version), http_state_t_to_string(resp->code));
#ifdef DEBUG_MODE
    for (size_t i = 0; i<resp->headers_count; i++) {
        log(DEBUG, "%.*s", resp->headers[i].len - 2, resp->headers[i].text);
    }
    log(DEBUG, "\n");
#endif

    switch (resp->http_version) {
        case HTTPv1_0: {
            evbuffer_add(output, STR_HTTPv1_0, strlen(STR_HTTPv1_0));
            break;
        }
        case HTTPv1_1: {
            evbuffer_add(output, STR_HTTPv1_1, strlen(STR_HTTPv1_1));
            break;
        }
        default: {
            evbuffer_add(output, STR_HTTPv1_0, strlen(STR_HTTPv1_0));
            break;
        }
    }
    evbuffer_add(output, " ", 1);
    switch (resp->code) {
        case OK: {
            evbuffer_add(output, STR_200_OK, strlen(STR_200_OK));
            break;
        }
        case BAD_REQUEST: {
            evbuffer_add(output, STR_400_BAD_REQUEST, strlen(STR_400_BAD_REQUEST));
            break;
        }
        case FORBIDDEN: {
            evbuffer_add(output, STR_403_FORBIDDEN, strlen(STR_403_FORBIDDEN));
            break;
        }
        case NOT_FOUND: {
            evbuffer_add(output, STR_404_NOT_FOUND, strlen(STR_404_NOT_FOUND));
            break;
        }
        case METHOD_NOT_ALLOWED: {
            evbuffer_add(output, STR_405_METHOD_NOT_ALLOWED, strlen(STR_405_METHOD_NOT_ALLOWED));
            break;
        }
        case INTERNAL_SERVER_ERROR: {
            evbuffer_add(output, STR_500_INTERNAL_SERVER_ERROR, strlen(STR_500_INTERNAL_SERVER_ERROR));
            break;
        }
        default: {
            evbuffer_add(output, STR_500_INTERNAL_SERVER_ERROR, strlen(STR_500_INTERNAL_SERVER_ERROR));
            break;
        }
    }
    evbuffer_add(output, "\r\n", 2);

    char connection_close = 1;
    for (size_t i = 0; i < resp->headers_count; i++) {
        if (strstr(resp->headers[i].text, STR_CONNECTION_KEEP_ALIVE_HEADER) != NULL) {
            connection_close = 0;
        }
        evbuffer_add(output, resp->headers[i].text, resp->headers[i].len);
    }
    evbuffer_add(output, "\r\n", 2);

    if (resp->file_to_send.fd > 0 && resp->file_to_send.len > 0) {
        evbuffer_add_file(output, resp->file_to_send.fd, 0, resp->file_to_send.len);
    }

    if(connection_close) {
        evbuffer_add_cb(output, socket_close_cb, bev);
    }
}

static void respond_with_err(struct bufferevent* bev, struct evbuffer* output, enum http_state_t code) {
    if (output == NULL) {
        log(ERROR, "Invalid function arguments");
        return;
    }

    struct http_response_t response = HTTP_RESPONSE_INITIALIZER;
    switch (code) {
        // NOTE: there are no default response for 200 OK
        case BAD_REQUEST: {
            break;
        }
        case FORBIDDEN: {
            break;
        }
        case NOT_FOUND: {
            break;
        }
        case METHOD_NOT_ALLOWED: {
            break;
        }
        case INTERNAL_SERVER_ERROR: {
            break;
        }
        default: {
            code = INTERNAL_SERVER_ERROR;
            break;
        }
    }

    const size_t headers_count = 4;
    struct http_header_t headers_arr[headers_count];
    size_t header_idx = 0;

    headers_arr[header_idx] = (struct http_header_t){
            STR_CONNECTION_CLOSE_HEADER,
            strlen(STR_CONNECTION_CLOSE_HEADER)
    };
    header_idx += 1;

    struct http_header_t date_header = HTTP_HEADER_INITIALIZER;
    char date_header_buffer[HTTP_HEADER_DEFAULT_BUFFER_SIZE];
    date_header.text = date_header_buffer;

    int build_result = build_date_header(&date_header);
    if (build_result < 0) {
        log(ERROR, "Unable to build Date header");
        headers_arr[header_idx] = (struct http_header_t){
                STR_DEFAULT_DATE_HEADER,
                strlen(STR_DEFAULT_DATE_HEADER)
        };
    } else {
        headers_arr[header_idx] = date_header;
    }
    header_idx++;

    headers_arr[header_idx] = (struct http_header_t){
        STR_CONTENT_LENGTH_ZERO_HEADER,
        strlen(STR_CONTENT_LENGTH_ZERO_HEADER)
    };
    header_idx++;

    headers_arr[header_idx] = (struct http_header_t){
            STR_SERVER_HEADER,
            strlen(STR_SERVER_HEADER)
    };
    header_idx++;

    response.code = code;
    response.http_version = HTTPv1_0; //TODO put in the config???
    response.file_to_send = (struct file_t)FILE_INITIALIZER;
    response.headers = headers_arr;
    response.headers_count = headers_count;

    log(DEBUG, "Response successfully built");
    respond(bev, output, &response);
}

static void conn_read_cb(struct bufferevent *bev, void *ctx) {
    /* This callback is invoked when there is data to read on bev */
    struct evbuffer* input = bufferevent_get_input(bev);
    struct evbuffer* output = bufferevent_get_output(bev);

    struct evbuffer_ptr req_headers_end = evbuffer_search(input, "\r\n\r\n", 4, NULL);
    if (req_headers_end.pos < 0) {
        log(WARNING, "Unable to find headers end, input buffer len %d bytes",evbuffer_get_length(input));
        respond_with_err(bev, output, BAD_REQUEST);
        return;
    }
    if (evbuffer_ptr_set(input, &req_headers_end, 4, EVBUFFER_PTR_ADD) < 0) {
        log(ERROR, "Unable to move req_headers_end evbuffer_ptr");
        respond_with_err(bev, output, INTERNAL_SERVER_ERROR);
        return;
    }

    char* req_str = malloc((size_t)req_headers_end.pos + 1);
    if (req_str == NULL) {
        log(ERROR, "Unable to allocate memory");
        respond_with_err(bev, output, INTERNAL_SERVER_ERROR);
        return;
    }
    req_str[req_headers_end.pos] = '\0';
    if (evbuffer_remove(input, req_str, (size_t)req_headers_end.pos) < 0) {
        log(ERROR, "Unable to copy data from input evbuffer");
        respond_with_err(bev, output, INTERNAL_SERVER_ERROR);
        free(req_str);
        return;
    }
    log(DEBUG, "req_str before parsing: <%s>", req_str);

    struct http_request_t req = HTTP_REQUEST_INITIALIZER;

    size_t headers_count = 0;
    char* headers_end = strstr(req_str, "\r\n\r\n");
    if (headers_end == NULL) {
        log(ERROR, "Unable to find headers end");
        respond_with_err(bev, output, BAD_REQUEST);
        free(req_str);
        return;
    }
    char* cursor = strstr(req_str, "\r\n");
    while (cursor != headers_end) {
        cursor += 2;
        headers_count++;
        cursor = strstr(cursor, "\r\n");
    }
    req.headers = malloc(headers_count * sizeof(struct http_header_t));
    if (req.headers == NULL) {
        log(ERROR, "Unable to allocate memory");
        respond_with_err(bev, output, INTERNAL_SERVER_ERROR);
        free(req_str);
        return;
    }
    req.headers_count = headers_count;
    log(DEBUG, "Headers count: %d", headers_count);

    enum http_state_t parse_result = parse_http_request(req_str, &req);
    switch (parse_result) {
        case OK: {
            log(INFO, "HTTP Request was parsed! METHOD: <%s>; URI: <%s>; VERSION: <%s>",
                    request_method_t_to_string(req.method),
                    req.URI,
                    http_version_t_to_string(req.http_version));
            break;
        }
        case BAD_REQUEST: {
            log(INFO, "HTTP Request was not parsed: BAD_REQUEST");
            respond_with_err(bev, output, BAD_REQUEST);
            free(req.headers);
            free(req_str);
            return;
        }
        case METHOD_NOT_ALLOWED: {
            log(INFO, "HTTP Request was not parsed: METHOD_NOT_ALLOWED");
            respond_with_err(bev, output, METHOD_NOT_ALLOWED);
            free(req.headers);
            free(req_str);
            return;
        }
        case INTERNAL_SERVER_ERROR: {
            log(ERROR, "HTTP Request was not parsed: INTERNAL_SERVER_ERROR");
            respond_with_err(bev, output, INTERNAL_SERVER_ERROR);
            free(req.headers);
            free(req_str);
            return;
        }
        default: {
            log(ERROR, "Unexpected http request parsing return code: %d", parse_result);
            respond_with_err(bev, output, INTERNAL_SERVER_ERROR);
            free(req.headers);
            free(req_str);
            return;
        }
    }

    struct http_response_t resp = HTTP_RESPONSE_INITIALIZER;
    const int resp_headers_count = 5;
    struct http_header_t headers[resp_headers_count];
    char resp_headers_buffer[resp_headers_count][HTTP_HEADER_DEFAULT_BUFFER_SIZE];
    for (size_t i = 0; i < resp_headers_count; i++) {
        headers[i].text = resp_headers_buffer[i];
    }
    resp.headers_count = resp_headers_count;
    resp.headers = headers;
    enum http_state_t build_result = build_http_response(&req, &resp);
    switch (build_result) {
        case OK: {
            log(INFO, "HTTP response was built!");
            break;
        }
        case FORBIDDEN: {
            log(INFO, "Can't build http response: access to file is forbidden");
            respond_with_err(bev, output, FORBIDDEN);
            free(req.headers);
            free(req_str);
            return;
        }
        case NOT_FOUND: {
            log(INFO, "Can't build http response: file was not found");
            respond_with_err(bev, output, NOT_FOUND);
            free(req.headers);
            free(req_str);
            return;
        }
        default: {
            log(ERROR, "Unexpected http response building return code: %d", build_result);
            respond_with_err(bev, output, INTERNAL_SERVER_ERROR);
            free(req.headers);
            free(req_str);
            if (resp.file_to_send.fd > 0) {
                close(resp.file_to_send.fd);
            }
            return;
        }
    }

    respond(bev, output, &resp); //TODO make correct connection header handling

    free(req.headers);
    free(req_str);
}

static void conn_event_cb(struct bufferevent *bev, short events, void *ctx) {
    log(DEBUG, "On conn_event_cb()");
    if (events & BEV_EVENT_ERROR) {
        log(ERROR, "Got some error on bufferevent: %s",strerror(errno));
        bufferevent_free(bev);
    }
}

static void accept_conn_cb(struct evconnlistener *listener,
                           evutil_socket_t fd, struct sockaddr *address, int socklen,
                           void *ctx) {
    /* We got a new connection! Set up a bufferevent for it */
    log(DEBUG, "On accept_conn_cb(), fd: %d", fd);
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, conn_read_cb, NULL, conn_event_cb, NULL);

    bufferevent_enable(bev, EV_READ|EV_WRITE);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx) {
    log(DEBUG, "On accept_error_cb()");
    int err = EVUTIL_SOCKET_ERROR();
    log(ERROR, "Got an error %d (%s) on the listener while accepting", err, evutil_socket_error_to_string(err));
}

int listen_and_serve(u_int16_t port) {
    struct event_base *base;
    struct evconnlistener *listener;
    struct sockaddr_in sin;

    base = event_base_new();
    if (!base) {
        log(FATAL, "Unable to open event base");
        return EXIT_FAILURE;
    }
    log(INFO, "libevent backend: %s", event_base_get_method(base));

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0);
    sin.sin_port = htons(port);

    listener = evconnlistener_new_bind(
            base,
            accept_conn_cb,
            NULL,
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE,
            -1,
            (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) {
        log(FATAL, "Couldn't create listener, ERRNO: %d %s", errno, strerror(errno));
        return EXIT_FAILURE;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);

#ifdef DEBUG_MODE
    log(DEBUG, "Listening socket fd: %d", evconnlistener_get_fd(listener));
#endif

    if(getuid() == 0) {
        log(DEBUG, "Dropping privilage");
        FILE* pp = popen("id " DEFAULT_USER "| sed 's/uid=//; s/(.*$//g'", "r");
        if (pp == NULL) {
            log(ERROR, "Unable to open pipe");
            return -1;
        }
        int uid = 0;
        fscanf(pp, "%d", &uid);
        fclose(pp);
        setuid(uid);
        if (errno != 0) {
            log(ERROR, "Error while dropping privilage: %s", strerror(errno));
            return -1;
        }
        log(INFO, "Privilage dropped to uid: %d", uid);
    }

    pid_t pid = 0;
    for (int i = 0; i < CPU_LIMIT - 1; i++) {
        switch(pid=fork()) {
            case -1: {
                log(ERROR, "Fork caused error: %s", strerror(errno));
            }
            case 0 : {
                event_base_dispatch(base);
                event_base_free(base);
            }
            default : {
                log(INFO, "Forked successfully, PID=%d", pid);
            }
        }
    }

    event_base_dispatch(base);
    event_base_free(base);
    return 0;
}
