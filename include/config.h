#ifndef HIGHLOADSERVER_CONFIG_H
#define HIGHLOADSERVER_CONFIG_H

void init_config(int cpu_limit_arg, const char* document_root_arg);
//Ручки, заданные через булевы переменные проверяются через #ifdef
//Для отключения просто закомментировать

//General settings
#define VERSION "1.1"
#define APP_NAME "FastHttpServer"
//#define DEBUG_MODE
#define DEFAULT_USER "httpd"
#define DEFAULT_PORT 80
int _get_cpu_limit(void);
#define CPU_LIMIT _get_cpu_limit()

//Logger settings
#define LOG_LEVEL 1 //0 = DEBUG ... 5 = FATAL
#define DO_COLOR_LOG
//#define LOG_FULL_FILE_PATH

//FileSystem settings
char* _get_document_root(void);
#define DOCUMENT_ROOT _get_document_root()

#endif //HIGHLOADSERVER_CONFIG_H
