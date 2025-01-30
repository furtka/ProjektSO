#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "hive_ipc.h"
#include "logger/logger.h"

#define handle_error(x)                                                               \
    if (!sigint && x == -1)                                                                      \
    {                                                                                 \
        log(LOG_LEVEL_ERROR, log_tag, "ERROR %s at %s\n", strerror(errno), __func__); \
        if (!sigint) try_clean_and_exit_with_error();                                              \
    }

void try_clean_and_exit_with_error();
void try_clean_and_exit();

volatile sig_atomic_t sigint;

#define STATE_INSIDE 0
#define STATE_OUTSIDE 1

int current_state;
int life_span;
int bee_id;
int bee_time_in_hive;
int bee_time_outside_hive;
int been_in_hive_counter = 0;

char *log_tag;

char *create_log_tag()
{
    char *tag = (char *)malloc(15);
    sprintf(tag, "BEE_%d", bee_id);
    return tag;
}

void parse_command_line_arguments(int argc, char *argv[])
{
    log(LOG_LEVEL_INFO, "BEE", "parsing input parameters");
    if (argc != 6)
    {
        printf("Usage: %s <bee_id> <life_span> <bee_time_in_hive> <bee_time_outside_hive> <is_inside>\n", argv[0]);
        exit(1);
    }

    bee_id = atoi(argv[1]);
    life_span = atoi(argv[2]);
    bee_time_in_hive = atoi(argv[3]);
    bee_time_outside_hive = atoi(argv[4]);
    int is_inside = atoi(argv[5]);
    if (is_inside)
    {
        current_state = STATE_INSIDE;
    }
    else
    {
        current_state = STATE_OUTSIDE;
    }

    log_tag = create_log_tag();
    log(LOG_LEVEL_INFO, log_tag, "end of parsing parameters bee_id=%d life_span=%d bee_time_in_hive=%d bee_time_outside_hive=%d is_inside=%d", bee_id, life_span, bee_time_in_hive, bee_time_outside_hive, is_inside);
}

void enter_hive()
{
    log(LOG_LEVEL_INFO, log_tag, "Want to enter the hive, waiting for room");
    int gate_id = rand() % GATES_NUMBER;
    handle_error(sem_wait(room_inside_semaphore));
    handle_error(sem_wait(gate_semaphore[gate_id]));
    log(LOG_LEVEL_INFO, log_tag, "Entering through the gate %d", gate_id);
    gate_message message;
    message.type = USED_GATE_TYPE;
    message.delta = 1;
    log(LOG_LEVEL_INFO, log_tag, "Sending message to gate %d", gate_id);
    handle_error(msgsnd(gate_message_queue[gate_id], &message, sizeof(int), 0));
    log(LOG_LEVEL_INFO, log_tag, "Waiting for ack from gate %d", gate_id);
    handle_error(msgrcv(gate_message_queue[gate_id], &message, sizeof(int), ACK_TYPE, 0));
    log(LOG_LEVEL_INFO, log_tag, "Received ack from gate %d", gate_id);
    current_state = STATE_INSIDE;

    handle_error(sem_post(gate_semaphore[gate_id]));
    log(LOG_LEVEL_INFO, log_tag, "bee is inside");
}

void leave_hive()
{
    log(LOG_LEVEL_INFO, log_tag, "Want to leave the hive");
    int gate_id = rand() % GATES_NUMBER;
    handle_error(sem_wait(gate_semaphore[gate_id]));
    log(LOG_LEVEL_INFO, log_tag, "Leaving through the gate %d", gate_id);
    gate_message message;
    message.type = USED_GATE_TYPE;
    message.delta = -1;
    log(LOG_LEVEL_INFO, log_tag, "Sending message to gate %d", gate_id);
    handle_error(msgsnd(gate_message_queue[gate_id], &message, sizeof(int), 0));
    log(LOG_LEVEL_INFO, log_tag, "Waiting for ack from gate %d", gate_id);
    handle_error(msgrcv(gate_message_queue[gate_id], &message, sizeof(int), ACK_TYPE, 0));
    log(LOG_LEVEL_INFO, log_tag, "Received ack from gate %d", gate_id);
    current_state = STATE_OUTSIDE;
    been_in_hive_counter++;
    handle_error(sem_post(room_inside_semaphore));
    handle_error(sem_post(gate_semaphore[gate_id]));
    log(LOG_LEVEL_INFO, log_tag, "bee is outside, been in hive %d/%d times", been_in_hive_counter, life_span);
}

void bee_lifecycle()
{
    if (current_state == STATE_OUTSIDE && !sigint)
    {
        enter_hive();
    }
    if (!sigint) sleep(bee_time_in_hive);
    if (current_state == STATE_INSIDE && !sigint)
    {
        leave_hive();
    }
    if (!sigint) sleep(bee_time_outside_hive);
}

void cleanup_resources()
{
    close_semaphores();
    close_logger();
    free(log_tag);
}

void try_clean_and_exit_with_error()
{
    cleanup_resources();
    exit(1);
}

void try_clean_and_exit()
{
    cleanup_resources();
    exit(0);
}


void handle_sigint(int singal)
{
    sigint = 1;
}

int main(int argc, char *argv[])
{
    init_logger();
    signal(SIGINT, handle_sigint);
    parse_command_line_arguments(argc, argv);
    handle_error(initialize_gate_message_queue());
    handle_error(open_semaphores(1));
    for (
        been_in_hive_counter = 0;
        been_in_hive_counter < life_span && !sigint;
        been_in_hive_counter++)
    {
        bee_lifecycle();
    }

    if (sigint)
    {
        log(LOG_LEVEL_INFO, log_tag, "Exiting bee due to SIGINT");
    }
    else
    {
        log(LOG_LEVEL_INFO, log_tag, "Dead");
    }

    try_clean_and_exit();
}
