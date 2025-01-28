#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>

#include "logger_internal.h"
#include "logger.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

void init_logger() 
{
    allocate();
}

void log(int level, char *tag, char *message, ...) 
{
    printf("BEGIN LOGGING\n");
    va_list args;
    va_start(args, message);
    char buffer[MAX_LOG_MESSAGE_SIZE + 1];
    buffer[MAX_LOG_MESSAGE_SIZE] = '\0';
    vsnprintf(buffer, MAX_LOG_MESSAGE_SIZE, message, args);
    va_end(args);

    char* tag_copy = malloc(strlen(tag) + 1);
    strcpy(tag_copy, tag);
    if (strlen(tag) > MAX_TAG_SIZE) 
    {
        tag_copy[MAX_TAG_SIZE] = '\0';
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    LogMessage log_message;
    log_message.log_timestamp_s = ts.tv_sec;
    log_message.log_timestamp_ns = ts.tv_nsec;
    log_message.log_level = level;
    strcpy(log_message.log_tag, tag_copy);
    strcpy(log_message.log_message, buffer);

    write_log(&log_message);

    free(tag_copy);
    printf("END LOGGING\n");
}

void close_logger() 
{
    deallocate_client();
}
