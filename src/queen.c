#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "logger/logger.h"
#include "hive_ipc.h"

int new_bee_interval;
int next_bee_id = 0;

void parse_command_line_arguments(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <T> <next_bee_id>\n", argv[0]);
        exit(1);
    }

    new_bee_interval = atoi(argv[1]);
    log(LOG_LEVEL_INFO, "QUEEN", "New bee interval is %d", new_bee_interval);
    next_bee_id = atoi(argv[2]);
    log(LOG_LEVEL_INFO, "QUEEN", "Next bee id is %d", next_bee_id);
}

volatile sig_atomic_t sigint = 0;

void handle_sigint(int signal)
{
    sigint = 1;
}

int create_new_bee()
{
    int pid = fork();
    if (pid == 0)
    {
        char *id = (char *)malloc(6);
        sprintf(id, "%d", next_bee_id);
        char *life_span = (char *)malloc(4);
        sprintf(life_span, "%d", rand() % 10 + 2);
        char *time_in_hive = (char *)malloc(4);
        sprintf(time_in_hive, "%d", rand() % 10 + 2);
        char *time_outside_hive = (char *)malloc(4);
        sprintf(time_outside_hive, "%d", rand() % 10 + 2);

        execl("./bin/bee", "./bin/bee", id, life_span, time_in_hive, time_in_hive, "1", NULL);
        log(LOG_LEVEL_ERROR, "QUEEN", "Error launching bee process, exiting...");
        close_logger();
        exit(1);
    }
    next_bee_id++;
    return pid;
}

RESULT queen_lifecycle()
{
    log(LOG_LEVEL_INFO, "QUEEN", "Queen lifecycle started, next bee id = %d", next_bee_id);
    sleep(new_bee_interval);
    log(LOG_LEVEL_INFO, "QUEEN", "Requesting new bee");
    handle_failure(request_queen_bee_born());
    log(LOG_LEVEL_INFO, "QUEEN", "Awaiting new bee allowance");
    handle_failure(await_queen_birth_allowance());
    log(LOG_LEVEL_INFO, "QUEEN", "Sending new bee confirmation");
    int pid = create_new_bee();
    handle_failure(send_queen_birth_confirmation(pid));
    return SUCCESS;
}

int main(int argc, char *argv[])
{
    init_logger();
    log(LOG_LEVEL_INFO, "QUEEN", "Starting queen");
    signal(SIGINT, handle_sigint);
    parse_command_line_arguments(argc, argv);
    initialize_gate_message_queue();

    RESULT result = SUCCESS;
    while (!sigint && (result = queen_lifecycle()) == SUCCESS)
        ;

    log(LOG_LEVEL_INFO, "QUEEN", "Exiting queen");
    close_logger();
}