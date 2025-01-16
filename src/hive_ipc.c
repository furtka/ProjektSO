#include "hive_ipc.h"
#include "logger.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define ENTER_REQUEST_TYPE 1
#define LEAVE_REQUEST_TYPE 2
#define enter_confirmation_type(gate_id) (3 + gate_id)
#define leave_confirmation_type(gate_id) (3 + GATES_NUMBER + gate_id)
#define allow_use_gate_type(bee_id) (3 + 2 * GATES_NUMBER + bee_id)

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

void initialize_gate_message_queue()
{
    log(LOG_LEVEL_DEBUG, "HIVE_IPC", "Initializizing gate message queue");

    key_t key = ftok("worker.c", 65);
    gate_message_queue = msgget(key, 0666 | IPC_CREAT);
    if (gate_message_queue == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "Error creating gate message queue");
        exit(1);
    }
}

void cleanup_gate_message_queue()
{
    log(LOG_LEVEL_DEBUG, "HIVE_IPC", "Cleaning up gate message queue");

    if (msgctl(gate_message_queue, IPC_RMID, NULL) == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "Error removing gate message queue");
        exit(1);
    }
}

int await_bee_request_leave()
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Waiting for bee to request to leave");

    message bee_request_leave;

    return msgrcv(gate_message_queue, &bee_request_leave, sizeof(int), LEAVE_REQUEST_TYPE, 0) == -1
        ? -1
        : bee_request_leave.data;
}

int await_bee_request_enter()
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Waiting for bee to request to enter");

    message bee_request_enter;

    return msgrcv(gate_message_queue, &bee_request_enter, sizeof(int), ENTER_REQUEST_TYPE, 0) == -1
        ? -1
        : bee_request_enter.data;
}

RESULT send_bee_allow_leave_message(int bee_id, int gate_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending allow leave message to bee %d", bee_id);

    message bee_allow_leave_message = {allow_use_gate_type(bee_id), gate_id};
    
    return msgsnd(gate_message_queue, &bee_allow_leave_message, sizeof(int), 0) == -1
        ? FAILURE
        : SUCCESS;
}

RESULT send_bee_allow_enter_message(int bee_id, int gate_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending allow enter message to bee %d", bee_id);

    message bee_allow_enter_message = {allow_use_gate_type(bee_id), gate_id};
    
    return msgsnd(gate_message_queue, &bee_allow_enter_message, sizeof(int), 0) == -1
        ? FAILURE
        : SUCCESS;
}

RESULT await_bee_leave_confirmation(int gate_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Waiting for bee to leave confirmation on gate %d", gate_id);

    message bee_confirmation_leave;
    return msgrcv(gate_message_queue, &bee_confirmation_leave, sizeof(int), leave_confirmation_type(gate_id), 0) == -1
        ? FAILURE
        : SUCCESS;
}

RESULT await_bee_enter_confirmation(int gate_id)
{
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Waiting for bee to enter confirmation on gate %d", gate_id);

    message bee_confirmation_enter;
    return msgrcv(gate_message_queue, &bee_confirmation_enter, sizeof(int), enter_confirmation_type(gate_id), 0) == -1
        ? FAILURE
        : SUCCESS;
}

RESULT request_enter(int bee_id) { 
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending request to enter by bee %d", bee_id);

    message bee_request_in = {ENTER_REQUEST_TYPE, bee_id};
    return msgsnd(gate_message_queue, &bee_request_in, sizeof(int), 0) == -1
        ? FAILURE
        : SUCCESS;
}

int await_use_gate_allowance(int bee_id) { 
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Waiting for allowance to use gate by bee %d", bee_id);

    message allowance;
    if (msgrcv(gate_message_queue, &allowance, sizeof(int), allow_use_gate_type(bee_id), 0) == -1)
    {
        return -1;
    }
    return allowance.data;
}

RESULT send_enter_confirmation(int gate_id) {
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending enter confirmation on gate %d", gate_id);

    message bee_confirmation_in_message = {enter_confirmation_type(gate_id), 0};
    return msgsnd(gate_message_queue, &bee_confirmation_in_message, sizeof(int), 0) == -1
        ? FAILURE
        : SUCCESS;
}

RESULT request_leave(int bee_id) { 
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending request to leave by bee %d", bee_id);

    message bee_request_out = {LEAVE_REQUEST_TYPE, bee_id};
    return msgsnd(gate_message_queue, &bee_request_out, sizeof(int), 0) == -1
        ? FAILURE
        : SUCCESS;
}

RESULT send_leave_confirmation(int gate_id) { 
    log(LOG_LEVEL_INFO, "HIVE_IPC", "Sending leave confirmation on gate %d", gate_id);

    message bee_confirmation_out_message = {leave_confirmation_type(gate_id), 0};
    return msgsnd(gate_message_queue, &bee_confirmation_out_message, sizeof(int), 0) == -1
        ? FAILURE
        : SUCCESS;
}