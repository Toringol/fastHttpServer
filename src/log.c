#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <memory.h>

#include "../include/log.h"
#include "../include/config.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_BOLD_TEXT "\033[1m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define DEBUG_TITLE   "[DEBU]"
#define INFO_TITLE    "[INFO]"
#define WARNING_TITLE "[WARN]"
#define ERROR_TITLE   "[ERRO]"
#define FATAL_TITLE   "[FATA]"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void do_log(FILE* stream, const char* color, const char* log_title,
            const char* file, int line, const char* fmt, va_list* argptr) {
    pthread_mutex_lock(&mutex);
#ifdef DO_COLOR_LOG
    fprintf(stream, color);
#endif
#ifdef LOG_FULL_FILE_PATH
    fprintf(stream, "%s <%s:%d> ", log_title, file, line);
#else
    fprintf(stream, "%s <%s:%d> ", log_title, strrchr(file, '/') + 1, line);
#endif
    vfprintf(stream, fmt, *argptr);
    fprintf(stream, "\n");
#ifdef DO_COLOR_LOG
    fprintf(stream, ANSI_COLOR_RESET);
#endif
    pthread_mutex_unlock(&mutex);
}

void _log(enum log_level_t log_level, const char* file, int line, const char* fmt, ...) {
#ifndef DEBUG_MODE
    if (log_level <LOG_LEVEL) {
        return;
    }
#endif
    va_list argptr;
    va_start(argptr, fmt);
    switch (log_level) {
        case DEBUG: {
#ifdef DEBUG_MODE
            do_log(stdout, ANSI_COLOR_GREEN, DEBUG_TITLE, file, line, fmt, &argptr);
#endif
            break;
        }
        case INFO: {
            do_log(stdout, ANSI_COLOR_RESET, INFO_TITLE, file, line, fmt, &argptr);
            break;
        }
        case WARNING: {
            do_log(stdout, ANSI_COLOR_YELLOW, WARNING_TITLE, file, line, fmt, &argptr);
            break;
        }
        case IMPORTANT: {
            //Log without title
            pthread_mutex_lock(&mutex);
#ifdef DO_COLOR_LOG
            fprintf(stdout, ANSI_COLOR_MAGENTA ANSI_BOLD_TEXT);
#endif
            vfprintf(stdout, fmt, argptr);
            fprintf(stdout, "\n");
#ifdef DO_COLOR_LOG
            fprintf(stdout, ANSI_COLOR_RESET);
#endif
            pthread_mutex_unlock(&mutex);
            break;
        }
        case ERROR: {
            do_log(stderr, ANSI_COLOR_RED, ERROR_TITLE, file, line, fmt, &argptr);
            break;
        }
        case FATAL: {
            do_log(stderr, ANSI_COLOR_RED ANSI_BOLD_TEXT, FATAL_TITLE, file, line, fmt, &argptr);
            va_end(argptr);
            exit(EXIT_FAILURE);
        }
        default: {
            _log(ERROR, file, line, "Unknown log level");
            break;
        }
    }
    va_end(argptr);
}
