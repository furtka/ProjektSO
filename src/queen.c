#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/msg.h>

#include "logger/logger.h"
#include "hive_ipc.h"

#define log_tag "QUEEN"
#define handle_error(x)                                                               \
    if (!sigint && x == -1)                                                                      \
    {                                                                                 \
        log(LOG_LEVEL_ERROR, log_tag, "ERROR %s at %s\n", strerror(errno), __func__); \
        if (!sigint) try_clean_and_exit_with_error();                                              \
    }

void try_clean_and_exit_with_error();
void try_clean_and_exit();

int new_bee_interval;
int next_bee_id = 0;

void parse_command_line_arguments(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <T> <next_bee_id>\n", argv[0]);
        exit(1);
    }

    new_bee_interval = atoi(argv[1]);
    log(LOG_LEVEL_INFO, "QUEEN", "New bee interval is %d", new_bee_interval);
}

volatile sig_atomic_t sigint = 0;

void handle_sigint(int signal)
{
    sigint = 1;
}

void queen_lifecycle()
{
    if (!sigint) sleep(new_bee_interval);
    log(LOG_LEVEL_INFO, "QUEEN", "Creating new bee, waiting for room inside");
    handle_error(sem_wait(room_inside_semaphore));

    queen_message message;
    message.type = GIVE_BIRTH;
    message.data = 0;

    log(LOG_LEVEL_INFO, "QUEEN", "Sending information to hive about new bee");
    handle_error(msgsnd(queen_message_queue, &message, sizeof(int), 0));

    log(LOG_LEVEL_INFO, "QUEEN", "Sent information to hive about new bee");
}

void try_clean_and_exit_with_error()
{
    close_semaphores();
    close_logger();
    exit(1);
}

void try_clean_and_exit()
{
    close_semaphores();
    close_logger();
    exit(0);
}

int main(int argc, char *argv[])
{
    init_logger();
    log(LOG_LEVEL_INFO, "QUEEN", "Starting queen");
    signal(SIGINT, handle_sigint);
    parse_command_line_arguments(argc, argv);
    initialize_queen_message_queue();
    open_semaphores(0);

    while (!sigint)
    {
        queen_lifecycle();
    }

    log(LOG_LEVEL_INFO, "QUEEN", "Exiting queen");

    try_clean_and_exit();
}