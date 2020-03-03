#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "../include/config.h"
#include "../include/server.h"
#include "../include/log.h"

int parse_config(const char *conf_path, int *cpu_limit, char **document_root) {
    FILE* conf_file = fopen(conf_path, "r");
    if (conf_file == NULL) {
        return -1;
    }

    bool num_cpu_inited = false;
    bool document_root_inited = false;

    const char* const key_cpu_limit = "cpu_limit \0";
    const char* const key_document_root = "document_root \0";

    char buffer[64];
    char* cursor = NULL;

    while (1) {
        if (num_cpu_inited && document_root_inited) break;
        if (!fgets(buffer, 64, conf_file)) return -1;

        cursor = strstr(buffer, key_cpu_limit);
        if (cursor) {
            log(DEBUG, "Found cpu_limit");
            cursor += strlen(key_cpu_limit);
            sscanf(cursor, "%d", cpu_limit);
            num_cpu_inited = true;
            continue;
        }

        cursor = strstr(buffer, key_document_root);
        if (cursor) {
            log(DEBUG, "Found document_root");
            cursor += strlen(key_document_root);
            strcpy(*document_root, cursor);
            char* path_end = strpbrk(*document_root, "\n #");
            if (path_end != NULL) {
                *path_end = '\0';
            }
            document_root_inited = true;
            continue;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        int cpu_limit = 0;
        char* document_root = malloc(4096);
        if(parse_config(argv[1], &cpu_limit, &document_root)) {
            log(FATAL, "%s", document_root);
            log(FATAL, "Unable to init config with .conf file");
        }
        init_config(cpu_limit, document_root);
        log(INFO, "httpd.conf parsed: document_root = %s, cpu_limit = %d", DOCUMENT_ROOT, CPU_LIMIT);
    } else {
        log(WARNING, ".conf config file does not passed, using defaults");
    }

    int port = DEFAULT_PORT;
    if (port<=0 || port>65535) {
        puts("Invalid port");
        return 1;
    }
    log(IMPORTANT, "%s v%s is listening on port %d", APP_NAME, VERSION, port);
    return listen_and_serve((u_int16_t)port);
}
