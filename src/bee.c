#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hive_ipc.h"
#include "logger/logger.h"

#define STATE_ENTERING 0
#define STATE_INSIDE 1
#define STATE_LEAVING 2
#define STATE_OUTSIDE 3

int current_state = STATE_OUTSIDE;
int been_inside_counter = 0;

int life_span;
int bee_id;
int bee_time_in_hive;
int bee_time_outside_hive;

char *log_tag;
char *logs_directory;

char *create_log_tag()
{
    char *tag = (char *)malloc(15);
    sprintf(tag, "BEE_%d", bee_id);
    return tag;
}

void parse_command_line_arguments(int argc, char *argv[])
{
    if (argc != 6)
    {
        printf("Usage: %s <bee_id> <life_span> <bee_time_in_hive> <bee_time_outside_hive> <logs_directory>\n", argv[0]);
        exit(1);
    }

    bee_id = atoi(argv[1]);
    life_span = atoi(argv[2]);
    bee_time_in_hive = atoi(argv[3]);
    bee_time_outside_hive = atoi(argv[4]);
    logs_directory = argv[5];

    log_tag = create_log_tag();
}

RESULT enter_hive()
{
    handle_failure(request_enter(bee_id));
    handle_result_as(gate_id, await_use_gate_allowance(bee_id));
    current_state = STATE_ENTERING;
    log(LOG_LEVEL_INFO, log_tag, "Bee is entering through gate %d", gate_id);
    handle_failure(send_enter_confirmation(gate_id));
    current_state = STATE_INSIDE;
    log(LOG_LEVEL_INFO, log_tag, "Bee is inside", bee_id);
    return SUCCESS;
}

RESULT leave_hive()
{
    handle_failure(request_leave(bee_id));
    handle_result_as(gate_id, await_use_gate_allowance(bee_id));
    log(LOG_LEVEL_INFO, log_tag, "Bee is leaving through gate %d", gate_id);
    current_state = STATE_LEAVING;
    handle_failure(send_leave_confirmation(gate_id));
    current_state = STATE_OUTSIDE;
    log(LOG_LEVEL_INFO, log_tag, "Bee is outside");
    sleep(bee_time_outside_hive);
}

RESULT bee_lifecycle()
{
    handle_failure(enter_hive());
    sleep(bee_time_in_hive);
    handle_failure(leave_hive());
    sleep(bee_time_outside_hive);
}

void cleanup_resources()
{
    close_logger();
    free(log_tag);
}

volatile sig_atomic_t stop;

void handle_sigint(int singal)
{
    stop = 1;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);
    parse_command_line_arguments(argc, argv);
    init_logger();
    initialize_gate_message_queue();

    RESULT bee_lifecycle_result = SUCCESS;
    for (
        been_inside_counter = 0;
        bee_lifecycle_result == SUCCESS && been_inside_counter < life_span && !stop;
        been_inside_counter++)
    {
        bee_lifecycle_result = bee_lifecycle();
    }

    if (bee_lifecycle_result == FAILURE)
    {
        log(LOG_LEVEL_ERROR, log_tag, "Bee failed to complete lifecycle");
    }
    else if (stop)
    {
        log(LOG_LEVEL_INFO, log_tag, "Bee is interrupted");
    }
    else
    {
        log(LOG_LEVEL_INFO, log_tag, "Bee is dead");
    }
    cleanup_resources();
    return 0;
}
