#include "hive_ipc.h"
#include "logger/logger.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/semaphore.h>
#include <sys/fcntl.h>

#define ENTER_REQUEST_TYPE 1
#define LEAVE_REQUEST_TYPE 2
#define QUEEN_BEE_BIRTH_REQUEST_TYPE 3
#define QUEEN_BEE_BIRTH_ALLOWANCE_TYPE 4
#define QUEEN_BEE_BIRTH_CONFIRMATION_TYPE 5
#define enter_confirmation_type(gate_id) (6 + gate_id)
#define leave_confirmation_type(gate_id) (6 + GATES_NUMBER + gate_id)
#define allow_use_gate_type(bee_id) (6 + 2 * GATES_NUMBER + bee_id)

#define SEMAPHORE_QUEUE_INSIDE "/semaphore_queue_inside"
#define SEMAPHORE_QUEUE_OUTSIDE "/semaphore_queue_outside"
#define SEMAPHORE_QUEEN "/semaphore_queen_inside"

#define MAX_QUEUE_SIZE 1

#define handle_msg_error(result)                                                       \
    if (result == -1)                                                                  \
    {                                                                                  \
        if (errno == EINTR)                                                            \
        {                                                                              \
            return INTERRUPTED;                                                        \
        }                                                                              \
        else                                                                           \
        {                                                                              \
            fprintf(stderr, "%s at %s\n", strerror(errno), __func__);                  \
            log(LOG_LEVEL_ERROR, "HIVE_IPC", "%s at %s\n", strerror(errno), __func__); \
            return FAILURE;                                                            \
        }                                                                              \
    }

typedef struct
{
    /**
     * REQUEST TO ENTER   = 1
     * REQUEST TO LEAVE   = 2
     * CONFIRMATION ENTER = gate_id + 3
     * CONFIRMATION LEAVE = gate_id + 3 + GATE_NUMBER
     * ALLOW USE GATE     = bee_id + 3 + 2 * GATES_NUMBER
     */
    long type;

    /**
     * Either bee_id or gate_id depending on context
     */
    int data;
} message;

int gate_message_queue;

sem_t *queue_inside_semaphore;
sem_t *queue_outside_semaphore;
sem_t *queen_semaphore;

RESULT initialize_gate_message_queue()
{
    log(LOG_LEVEL_DEBUG, "HIVE_IPC", "Initializizing gate message queue");

    key_t key = ftok("worker.c", 65);
    gate_message_queue = msgget(key, 0666 | IPC_CREAT);
    if (gate_message_queue == -1)
    {

        fprintf(stderr, "%s at %s\n", strerror(errno), __func__);
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "%s at %s\n", strerror(errno), __func__);
        exit(1);
    }

    queue_inside_semaphore = sem_open(SEMAPHORE_QUEUE_INSIDE, O_CREAT, 0644, MAX_QUEUE_SIZE);
    if (queue_inside_semaphore == SEM_FAILED)
    {
        fprintf(stderr, "%s at %s\n", strerror(errno), __func__);
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "%s at %s\n", strerror(errno), __func__);
        exit(1);
    }

    queue_outside_semaphore = sem_open(SEMAPHORE_QUEUE_OUTSIDE, O_CREAT, 0644, MAX_QUEUE_SIZE);
    if (queue_outside_semaphore == SEM_FAILED)
    {
        fprintf(stderr, "%s at %s\n", strerror(errno), __func__);
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "%s at %s\n", strerror(errno), __func__);
        exit(1);
    }

    queen_semaphore = sem_open(SEMAPHORE_QUEEN, O_CREAT, 0644, MAX_QUEUE_SIZE);
    if (queen_semaphore == SEM_FAILED)
    {
        fprintf(stderr, "%s at %s\n", strerror(errno), __func__);
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "%s at %s\n", strerror(errno), __func__);
        exit(1);
    }

    return SUCCESS;
}

RESULT cleanup_gate_message_queue()
{
    log(LOG_LEVEL_DEBUG, "HIVE_IPC", "Cleaning up gate message queue");

    handle_msg_error(msgctl(gate_message_queue, IPC_RMID, NULL));

    handle_msg_error(sem_close(queue_inside_semaphore));
    handle_msg_error(sem_unlink(SEMAPHORE_QUEUE_INSIDE));

    handle_msg_error(sem_close(queue_outside_semaphore));
    handle_msg_error(sem_unlink(SEMAPHORE_QUEUE_OUTSIDE))

    handle_msg_error(sem_close(queen_semaphore));
    handle_msg_error(sem_unlink(SEMAPHORE_QUEEN));

    return SUCCESS;
}

int await_bee_request_leave()
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Waiting for bee to request to leave");

    message bee_request_leave;

    handle_msg_error(
        msgrcv(gate_message_queue, &bee_request_leave, sizeof(int), LEAVE_REQUEST_TYPE, 0));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] TOOK FROM QUEUE");
    return bee_request_leave.data;
}

int await_bee_request_enter()
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Waiting for bee to request to enter");

    message bee_request_enter;

    handle_msg_error(
        msgrcv(gate_message_queue, &bee_request_enter, sizeof(int), ENTER_REQUEST_TYPE, 0));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] TOOK FROM QUEUE");
    return bee_request_enter.data;
}

RESULT send_bee_allow_leave_message(int bee_id, int gate_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending allow leave message to bee %d", bee_id);

    message bee_allow_leave_message = {allow_use_gate_type(bee_id), gate_id};

    handle_msg_error(
        msgsnd(gate_message_queue, &bee_allow_leave_message, sizeof(int), 0));

    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] ADDED TO QUEUE");

    return SUCCESS;
}

RESULT send_bee_allow_enter_message(int bee_id, int gate_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending allow enter message to bee %d", bee_id);

    message bee_allow_enter_message = {allow_use_gate_type(bee_id), gate_id};

    handle_msg_error(
        msgsnd(gate_message_queue, &bee_allow_enter_message, sizeof(int), 0))

    return SUCCESS;
}

RESULT await_bee_leave_confirmation(int gate_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Waiting for bee to leave confirmation on gate %d", gate_id);

    message bee_confirmation_leave;

    handle_msg_error(
        msgrcv(gate_message_queue, &bee_confirmation_leave, sizeof(int), leave_confirmation_type(gate_id), 0));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] TOOK FROM QUEUE");
    sem_post(queue_inside_semaphore);
    return SUCCESS;
}

RESULT await_bee_enter_confirmation(int gate_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Waiting for bee to enter confirmation on gate %d", gate_id);

    message bee_confirmation_enter;

    handle_msg_error(
        msgrcv(gate_message_queue, &bee_confirmation_enter, sizeof(int), enter_confirmation_type(gate_id), 0));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] TOOK FROM QUEUE");
    sem_post(queue_outside_semaphore);

    return SUCCESS;
}

RESULT request_enter(int bee_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Bee %d is wating to get in line to enter", bee_id);
    sem_wait(queue_outside_semaphore);
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending request to enter by bee %d", bee_id);

    message bee_request_in = {ENTER_REQUEST_TYPE, bee_id};

    handle_msg_error(
        msgsnd(gate_message_queue, &bee_request_in, sizeof(int), IPC_NOWAIT));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] ADDED TO QUEUE");
    return SUCCESS;
}

int await_use_gate_allowance(int bee_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Waiting for allowance to use gate by bee %d", bee_id);

    message allowance;

    handle_msg_error(
        msgrcv(gate_message_queue, &allowance, sizeof(int), allow_use_gate_type(bee_id), 0))
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] TOOK FROM QUEUE");
        return allowance.data;
}

RESULT send_enter_confirmation(int gate_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending enter confirmation on gate %d", gate_id);

    message bee_confirmation_in_message = {enter_confirmation_type(gate_id), 0};

    handle_msg_error(
        msgsnd(gate_message_queue, &bee_confirmation_in_message, sizeof(int), IPC_NOWAIT));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] ADDED TO QUEUE");
    return SUCCESS;
}

RESULT request_leave(int bee_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Bee %d is wating to get in line to leave", bee_id);
    sem_wait(queue_inside_semaphore);
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending request to leave by bee %d", bee_id);

    message bee_request_out = {LEAVE_REQUEST_TYPE, bee_id};

    handle_msg_error(
        msgsnd(gate_message_queue, &bee_request_out, sizeof(int), IPC_NOWAIT));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] ADDED TO QUEUE");
    return SUCCESS;
}

RESULT send_leave_confirmation(int gate_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending leave confirmation on gate %d", gate_id);

    message bee_confirmation_out_message = {leave_confirmation_type(gate_id), 0};

    handle_msg_error(
        msgsnd(gate_message_queue, &bee_confirmation_out_message, sizeof(int), IPC_NOWAIT));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] ADDED TO QUEUE");
    return SUCCESS;
}

RESULT await_queen_birth_request()
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[QUEEN] Waiting for queen birth request");

    message queen_birth_request;

    handle_msg_error(
        msgrcv(gate_message_queue, &queen_birth_request, sizeof(int), QUEEN_BEE_BIRTH_REQUEST_TYPE, 0));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] TOOK FROM QUEUE");
    return SUCCESS;
}

RESULT send_queen_birth_allowance()
{   
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[QUEEN] Sending queen birth allowance");

    message queen_birth_allowance = {QUEEN_BEE_BIRTH_ALLOWANCE_TYPE, 0};

    handle_msg_error(
        msgsnd(gate_message_queue, &queen_birth_allowance, sizeof(int), IPC_NOWAIT));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] ADDED TO QUEUE");
    return SUCCESS;
}

int await_queen_birth_confirmation()
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[QUEEN] Waiting for queen birth confirmation");

    message queen_birth_confirmation;

    handle_msg_error(
        msgrcv(gate_message_queue, &queen_birth_confirmation, sizeof(int), QUEEN_BEE_BIRTH_CONFIRMATION_TYPE, 0));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] TOOK FROM QUEUE");
    sem_post(queen_semaphore);
    return queen_birth_confirmation.data;
}

RESULT request_queen_give_birth()
{
    sem_wait(queen_semaphore);
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[QUEEN] Sending request from queen to give birth");

    message queen_birth_request = {QUEEN_BEE_BIRTH_REQUEST_TYPE, 0};

    handle_msg_error(
        msgsnd(gate_message_queue, &queen_birth_request, sizeof(int), IPC_NOWAIT));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] ADDED TO QUEUE");
    return SUCCESS;
}

RESULT await_queen_birth_allowance()
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[QUEEN] Waiting for queen birth allowance");

    message queen_birth_allowance;

    handle_msg_error(
        msgrcv(gate_message_queue, &queen_birth_allowance, sizeof(int), QUEEN_BEE_BIRTH_ALLOWANCE_TYPE, 0));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] TOOK FROM QUEUE");
    return SUCCESS;
}

RESULT send_queen_birth_confirmation(int pid)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[QUEEN] Sending queen birth confirmation");

    message queen_birth_confirmation = {QUEEN_BEE_BIRTH_CONFIRMATION_TYPE, pid};

    handle_msg_error(
        msgsnd(gate_message_queue, &queen_birth_confirmation, sizeof(int), IPC_NOWAIT));
    log(LOG_LEVEL_INFO, "HIVE_IPC", "[COUNT] ADDED TO QUEUE");
    return SUCCESS;
}