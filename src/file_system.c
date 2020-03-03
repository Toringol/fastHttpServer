#include <zconf.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../include/file_system.h"
#include "../include/log.h"
#include "../include/config.h"

char* const STR_MIME_APPLICATION_OCTET_STREAM = "application/octet-stream\0";
char* const STR_MIME_TEXT_HTML = "text/html\0";
char* const STR_MIME_TEXT_CSS = "text/css\0";
char* const STR_MIME_APPLICATION_JAVASCRIPT = "application/javascript\0";
char* const STR_MIME_IMAGE_JPEG = "image/jpeg\0";
char* const STR_MIME_IMAGE_PNG = "image/png\0";
char* const STR_MIME_IMAGE_GIF = "image/gif\0";
char* const STR_MIME_APPLICATION_X_SHOCKWAVE_FLASH = "application/x-shockwave-flash\0";

const char* const EXT_HTML = ".html\0";
const char* const EXT_CSS = ".css\0";
const char* const EXT_JS = ".js\0";
const char* const EXT_JPG = ".jpg\0";
const char* const EXT_JPEG = ".jpeg\0";
const char* const EXT_PNG = ".png\0";
const char* const EXT_GIF = ".gif\0";
const char* const EXT_SWF = ".swf\0";

char* mime_type_to_str(enum mime_t mime_type) {
    switch(mime_type) {
        case MIME_TYPE_APPLICATION_OCTET_STREAM: {
            return STR_MIME_APPLICATION_OCTET_STREAM;
        }
        case MIME_TYPE_TEXT_HTML: {
            return STR_MIME_TEXT_HTML;
        }
        case MIME_TYPE_TEXT_CSS: {
            return STR_MIME_TEXT_CSS;
        }
        case MIME_TYPE_APPLICATION_JAVASCRIPT: {
            return STR_MIME_APPLICATION_JAVASCRIPT;
        }
        case MIME_TYPE_IMAGE_JPEG: {
            return STR_MIME_IMAGE_JPEG;
        }
        case MIME_TYPE_IMAGE_PNG: {
            return STR_MIME_IMAGE_PNG;
        }
        case MIME_TYPE_IMAGE_GIF: {
            return STR_MIME_IMAGE_GIF;
        }
        case MIME_TYPE_APPLICATION_X_SHOCKWAVE_FLASH: {
            return STR_MIME_APPLICATION_X_SHOCKWAVE_FLASH;
        }
        default: {
            log(WARNING, "Unknown mime type: %d", mime_type);
            return STR_MIME_APPLICATION_OCTET_STREAM;
        }
    }
}

static enum file_state_t errno_to_file_state(int err_no) {
    switch (err_no) {
        case EFAULT:
        case ELOOP:
        case ENAMETOOLONG:
        case ENOENT:
        case ENOTDIR: {
            return FILE_STATE_NOT_FOUND;
        }
        case EACCES: {
            return FILE_STATE_FORBIDDEN;
        }
        default: {
            return FILE_STATE_INTERNAL_ERROR;
        }
    }
}

static enum mime_t parse_mime_type(const char* file_extension) {
    if (file_extension == NULL) {
        log(ERROR, "Invalid function arguments");
        return MIME_TYPE_APPLICATION_OCTET_STREAM;
    }
    if (strcmp(file_extension, EXT_HTML) == 0) {
        return MIME_TYPE_TEXT_HTML;
    } else if (strcmp(file_extension, EXT_CSS) == 0) {
        return MIME_TYPE_TEXT_CSS;
    } else if (strcmp(file_extension, EXT_JS) == 0) {
        return MIME_TYPE_APPLICATION_JAVASCRIPT;
    } else if (strcmp(file_extension, EXT_JPG) == 0) {
        return MIME_TYPE_IMAGE_JPEG;
    } else if (strcmp(file_extension, EXT_JPEG) == 0) {
        return MIME_TYPE_IMAGE_JPEG;
    } else if (strcmp(file_extension, EXT_PNG) == 0) {
        return MIME_TYPE_IMAGE_PNG;
    } else if (strcmp(file_extension, EXT_GIF) == 0) {
        return MIME_TYPE_IMAGE_GIF;
    } else if (strcmp(file_extension, EXT_SWF) == 0) {
        return MIME_TYPE_APPLICATION_X_SHOCKWAVE_FLASH;
    } else {
        log(WARNING, "Unknown file extension: %s", file_extension);
        return MIME_TYPE_APPLICATION_OCTET_STREAM;
    }
}

enum file_state_t inspect_file(char* path, struct file_t* file, bool should_get_fd) {
    if (path == NULL || file == NULL) {
        log(ERROR, "Invalid function arguments");
        return -1;
    }

    char absolute_path[4096] = "\0";
    strcpy(absolute_path, DOCUMENT_ROOT);
    strcat(absolute_path, path);
    if (absolute_path[strlen(absolute_path) - 1] == '/') {
        absolute_path[strlen(absolute_path) - 1] = '\0';
    }
    log(DEBUG,"Absolute path: %s", absolute_path);

    if(strstr(absolute_path, "/..") != NULL) {
        return FILE_STATE_FORBIDDEN;
    }

    int fd = open(absolute_path, O_RDONLY | O_NONBLOCK | O_DIRECTORY);
    if (errno == ENOTDIR) {
        log(DEBUG, "Absolute path is a regular file");
        fd = open(absolute_path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            log(ERROR, "Unable to open file: %s", strerror(errno));
            return errno_to_file_state(errno);
        }
    } else if (fd < 0) {
        log(ERROR, "Unable to open file by absolute path: %s", strerror(errno));
        return errno_to_file_state(errno);
    } else {
        log(DEBUG, "Absolute path points to directory");
        if (close(fd) < 0) {
            log(ERROR, "Unable to close directory fd: %s", strerror(errno));
            return errno_to_file_state(errno);
        }

        strcat(absolute_path, INDEX_FILE_NAME);
        log(DEBUG, "Absolute path: %s", absolute_path);

        fd = open(absolute_path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            log(ERROR, "Unable to open file: %s", strerror(errno));
            return FILE_STATE_FORBIDDEN;
        }
    }

    ssize_t file_len = lseek(fd, 0, SEEK_END);
    if (file_len < 0) {
        log(ERROR, "Unable to get file size via lseek: %s", strerror(errno));
        return errno_to_file_state(errno);
    }
    log(DEBUG, "File length: %d", file_len);

    file->path = absolute_path;
    file->len = (size_t)file_len;

    char* file_extension = strrchr(absolute_path, '.');
    if (file_extension == NULL) {
        file->mime_type = MIME_TYPE_APPLICATION_OCTET_STREAM;
    } else {
        file->mime_type = parse_mime_type(file_extension);
    }

    if (should_get_fd) {
        file->fd = fd;
    }

    return FILE_STATE_OK;
}
