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
     * type = 1: request to enter
     * type = 2: request to leave
     * type = 3 + gate_id: confirmation of entering by the gate_id gate
     * type = 3 + gate_number + gate_id: confirmation of leaving by the gate_id gate
     * type = 3 + 2 * GATES_NUMBER + bee_id: confirmation of allowing the bee_id to use the gate
     */
    long type;

    /**
     * either bee_id or gate_id depending on the type
     * if type = 1 or 2, then data is bee_id
     */
    int data;
} message;

int gate_message_queue;

void initialize_gate_message_queue()
{
    log(LOG_LEVEL_DEBUG, "HIVE_IPC", "Initializizing gate message q");

    key_t key = ftok("worker.c", 65);
    gate_message_queue = msgget(key, 0666 | IPC_CREAT);
}

void cleanup_gate_message_queue()
{
    log(LOG_LEVEL_DEBUG, "HIVE_IPC", "Cleaning up gate message queue");

    msgctl(gate_message_queue, IPC_RMID, NULL);
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