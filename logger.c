#include "logger.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>



void log(int level, char *tag, char *message, ...)
{
    // if (is_log_level(level))
    // {
        va_list args;
        va_start(args, message);

        printf("[%s] ", tag);
        fflush(stdout);
        vprintf(message, args);
        printf("\n");
        fflush(stdout);

        va_end(args);


    // }
}
