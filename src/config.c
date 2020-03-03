#include <stdbool.h>
#include <pthread.h>
#include <memory.h>
#include "../include/config.h"

int cpu_limit = 1;
int _get_cpu_limit(void) {
    return cpu_limit;
}

char document_root[4096] = "\0";
char* _get_document_root(void) {
    return document_root;
}

bool init_func_called = false;
pthread_mutex_t init_func_mutex = PTHREAD_MUTEX_INITIALIZER;
void init_config(int cpu_limit_arg, const char* document_root_arg) {
    pthread_mutex_lock(&init_func_mutex);
    if (init_func_called) {
        pthread_mutex_unlock(&init_func_mutex);
        return;
    }
    init_func_called = true;
    cpu_limit = cpu_limit_arg;
    strcpy(document_root, document_root_arg);
    pthread_mutex_unlock(&init_func_mutex);
}
