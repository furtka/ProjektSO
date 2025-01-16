#include "logger.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

FILE* log_file;

// TODO: make error handling better
void init_logger(char *logs_directory, char *log_filepath)
{
    if (mkdir(logs_directory, 0755) != 0 && errno != EEXIST)
    {
        printf("Error creating logs directory\n");
        exit(1);
    }
    char *log_full_path = (char *)malloc(strlen(logs_directory) + strlen(log_filepath) + 2);
    sprintf(log_full_path, "%s/%s", logs_directory, log_filepath);
    log_file = fopen(log_full_path, "w");
    if (log_file == NULL)
    {
        printf("Error creating log file\n");
        printf("%s\n", log_full_path);
        exit(1);
    }
    free(log_full_path);
}

void log(int level, char *tag, char *format, ...)
{
    if (!format)
    {
        return;
    }

    va_list args;
    va_start(args, format);

    size_t size = vsnprintf(NULL, 0, format, args) + 1;

    va_end(args);

    char *result = (char *)malloc(size);
    if (!result)
    {
        return;
    }

    va_start(args, format);
    vsnprintf(result, size, format, args);
    va_end(args);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    if (log_file != NULL)
    {
        fprintf(log_file, "[%ld.%ld] [%s]: %s\n", ts.tv_sec, ts.tv_nsec, tag, result);
    }
    else 
    {
        printf("[%ld.%ld] [%s]: %s\n", ts.tv_sec, ts.tv_nsec, tag, result);
    }
    free(result);
}

void close_logger()
{
    if (log_file != NULL)
    {
        fclose(log_file);
    }
}