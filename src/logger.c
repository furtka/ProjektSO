#include "logger.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void log(int level, char *tag, char *format, ...)
{
    if (!format) {
        return ;
    }

    va_list args;
    va_start(args, format);

    size_t size = vsnprintf(NULL, 0, format, args) + 1; 

    va_end(args);

    char* result = (char*)malloc(size);
    if (!result) {
        return ; 
    }

    va_start(args, format);
    vsnprintf(result, size, format, args);
    va_end(args);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    printf("[%ld.%ld] [%s]: %s\n", ts.tv_sec, ts.tv_nsec, tag, result);
    free(result);
}
