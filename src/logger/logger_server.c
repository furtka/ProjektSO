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
#include <signal.h>

#include "logger_internal.h"

volatile sig_atomic_t sigint = 0;

void handle_sigint(int sig)
{
    sigint = 1;
}

int main()
{
    struct timespec ts;
    allocate();
    signal(SIGINT, handle_sigint);

    clock_gettime(CLOCK_REALTIME, &ts);
    printf("[%ld.%ld] LOG_SERVER Server started\n", ts.tv_sec, ts.tv_nsec);

    while (!sigint)
    {
        LogMessage *log_message = read_log();

        printf(
            "[%ld.%ld] %-10s [PID=%d] %s\n",
            log_message->log_timestamp_s,
            log_message->log_timestamp_ns,
            log_message->log_tag,
            log_message->pid,
            log_message->log_message);

        free(log_message);
    }


    clock_gettime(CLOCK_REALTIME, &ts);
    printf("[%ld.%ld] LOG_SERVER Exiting...\n", ts.tv_sec, ts.tv_nsec);

    deallocate_server();

    return 0;
}
