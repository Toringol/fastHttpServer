#ifndef HIGHLOADSERVER_LOG_H
#define HIGHLOADSERVER_LOG_H

enum log_level_t {
    DEBUG,
    INFO,
    WARNING,
    IMPORTANT,
    ERROR,
    FATAL
};

void _log(enum log_level_t log_level, const char* file, int line, const char* fmt, ...);
#define log(log_level, fmt, ...) _log(log_level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
//TODO develop timing profiling

#endif //HIGHLOADSERVER_LOG_H
