#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "hive_ipc.h"
#include "logger.h"

#define STATE_ENTERING 0
#define STATE_INSIDE 1
#define STATE_LEAVING 2
#define STATE_OUTSIDE 3

// TOOD: allow to stear this from the command line argument
int current_state = STATE_OUTSIDE;
int been_inside_counter = 0;

int life_span;
int bee_id;
int bee_time_in_hive;
int bee_time_outside_hive;

char *log_tag;

char* create_log_tag()
{
    char *tag = (char *)malloc(11);
    if (bee_id > 99999) {
        bee_id = 99999;
    }
    sprintf(tag, "BEE %d", bee_id);
    return tag;
}

void parse_command_line_arguments(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Usage: %s <bee_id> <life_span> <bee_time_in_hive> <bee_time_outside_hive>\n", argv[0]);
        exit(1);
    }

    bee_id = atoi(argv[1]);
    life_span = atoi(argv[2]);
    bee_time_in_hive = atoi(argv[3]);
    bee_time_outside_hive = atoi(argv[4]);

    log_tag = create_log_tag();
}

void enter_hive()
{
    if (request_enter(bee_id) == FAILURE)
    {
        log(LOG_LEVEL_ERROR, log_tag, "error requesting enter");
        exit(1);
    }

    int gate_id = await_use_gate_allowance(bee_id);
    if (gate_id < 0)
    {
        log(LOG_LEVEL_ERROR, log_tag, "error awaiting use gate allowance");
        exit(2);
    }
    current_state = STATE_ENTERING;
    log(LOG_LEVEL_INFO, log_tag, "Bee is entering through gate %d", gate_id);

    if (send_enter_confirmation(gate_id) == FAILURE)
    {
        log(LOG_LEVEL_ERROR, log_tag, "error sending enter confirmation");
        exit(3);
    }
    current_state = STATE_INSIDE;
    log(LOG_LEVEL_INFO, log_tag, "Bee is inside", bee_id);
}

void leave_hive()
{
    if (request_leave(bee_id) == FAILURE)
    {
        log(LOG_LEVEL_ERROR, log_tag, "error requesting leave");
        exit(4);
    }

    int gate_id = await_use_gate_allowance(bee_id);
    if (gate_id < 0)
    {
        log(LOG_LEVEL_ERROR, log_tag, "error awaiting use gate allowance to leave");
        exit(5);
    }
    log(LOG_LEVEL_INFO, log_tag, "Bee is leaving through gate %d", gate_id);

    current_state = STATE_LEAVING;

    if (send_leave_confirmation(gate_id) == FAILURE)
    {
        log(LOG_LEVEL_ERROR, log_tag, "error sending leave confirmation");
        exit(6);
    }
    current_state = STATE_OUTSIDE;
    log(LOG_LEVEL_INFO, log_tag, "Bee is outside");

    sleep(bee_time_outside_hive);
}

void bee_lifecycle()
{
    enter_hive();
    sleep(bee_time_in_hive);
    leave_hive();
    sleep(bee_time_outside_hive);
}

int main(int argc, char *argv[])
{
    parse_command_line_arguments(argc, argv);
    initialize_gate_message_queue();

    for (been_inside_counter = 0; been_inside_counter < life_span; been_inside_counter++)
    {
        bee_lifecycle();
    }

    log(LOG_LEVEL_INFO, log_tag, "Bee is dead");
    printf("asdf");
    exit(0);
    return 0;
}