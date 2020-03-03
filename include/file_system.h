#ifndef HIGHLOADSERVER_FILE_SYSTEM_H
#define HIGHLOADSERVER_FILE_SYSTEM_H

#include <stdint-gcc.h>

enum mime_t {
    MIME_TYPE_APPLICATION_OCTET_STREAM,
    MIME_TYPE_TEXT_HTML,
    MIME_TYPE_TEXT_CSS,
    MIME_TYPE_APPLICATION_JAVASCRIPT,
    MIME_TYPE_IMAGE_JPEG,
    MIME_TYPE_IMAGE_PNG,
    MIME_TYPE_IMAGE_GIF,
    MIME_TYPE_APPLICATION_X_SHOCKWAVE_FLASH
};

char* mime_type_to_str(enum mime_t mime_type);

struct file_t {
    char* path;
    int64_t len;
    int fd;
    enum mime_t mime_type;
};
#define FILE_INITIALIZER {NULL, -1, -1, MIME_TYPE_APPLICATION_OCTET_STREAM}

enum file_state_t {
    FILE_STATE_INTERNAL_ERROR,
    FILE_STATE_FORBIDDEN,
    FILE_STATE_NOT_FOUND,
    FILE_STATE_OK
};

#define INDEX_FILE_NAME "/index.html\0"

enum file_state_t inspect_file(char* path, struct file_t* file, _Bool should_get_fd);

#endif //HIGHLOADSERVER_FILE_SYSTEM_H
