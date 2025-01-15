#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define STATE_ENTERING 0
#define STATE_INSIDE 1
#define STATE_LEAVING 2
#define STATE_OUTSIDE 3

#define GATES_NUMBER 1

#define ENTER_REQUEST_TYPE 1
#define LEAVE_REQUEST_TYPE 2
#define enter_confirmation_type(gate_id) (3 + gate_id)
#define leave_confirmation_type(gate_id) (3 + GATES_NUMBER + gate_id)
#define allow_use_gate_type(bee_id) (3 + 2 * GATES_NUMBER + bee_id)

int gate_message_queue;

// TODO: this should be either 0 or 1 depending on the starting location of the
//       bee. It should be set based on the command line arguments.
int been_inside_counter = 0;

// TODO: this should be set based on the command line arguments.
int life_span = 10;

// TODO: this should be set based on the command line argunment
int current_state = STATE_OUTSIDE;

// TODO: this should be set based on the command line argunment
int bee_id = 54;

typedef struct
{
    /**
     * type = 1: request to enter
     * type = 2: request to leave
     * type = 3 + gate_id: confirmation of entering by the gate_id gate
     * type = 3 + gate_number + gate_id: confirmation of leaving by the gate_id gate
     */
    long type;

    /**
     * either bee_id or gate_id depending on the type
     * if type = 1 or 2, then data is bee_id
     * if type = 3 or 4, then data is gate_id
     */
    int data;
} message;

int main(int argc, char *argv[])
{
    key_t key = ftok("worker.c", 65);
    gate_message_queue = msgget(key, 0666 | IPC_CREAT);

    sleep(1);

    for (been_inside_counter = 0; been_inside_counter < life_span; been_inside_counter++)
    {
        printf("attempting to send request to enter\n");
        message bee_request_in = {ENTER_REQUEST_TYPE, bee_id};
        if (msgsnd(gate_message_queue, &bee_request_in, sizeof(int), 0) == -1)
        {
            printf("error sending enter request\n");
            return 1;
        }

        printf("attempting to receive allowance to enter\n");
        message allowance;
        if (msgrcv(gate_message_queue, &allowance, sizeof(int), allow_use_gate_type(bee_id), 0) == -1)
        {
            printf("error receiving allowance\n");
            return 1;
        }
        int gate_id = allowance.data;
        current_state = STATE_ENTERING;
        printf("Bee %d is entering the gate %d\n", bee_id, gate_id);

        message bee_confirmation_in_message = {enter_confirmation_type(gate_id), 0};
        if (msgsnd(gate_message_queue, &bee_confirmation_in_message, sizeof(int), 0) == -1)
        {
            printf("error sending enter confirmation\n");
            return 1;
        }
        current_state = STATE_INSIDE;
        printf("Bee %d is inside\n", bee_id);

        sleep(rand() % 10); // TODO: this should be read from the command line arguments

        printf("attempting to send request to leave\n");
        message bee_request_out = {LEAVE_REQUEST_TYPE, bee_id};
        if (msgsnd(gate_message_queue, &bee_request_out, sizeof(int), 0) == -1)
        {
            printf("error sending leave request\n");
            return 1;
        }

        printf("attempting to receive allowance to leave\n");
        message bee_allow_out_message;
        if (msgrcv(gate_message_queue, &bee_allow_out_message, sizeof(int), allow_use_gate_type(bee_id), 0) == -1)
        {
            printf("error receiving allowance\n");
            return 1;
        }
        gate_id = bee_allow_out_message.data;
        current_state = STATE_LEAVING;
        printf("Bee %d is leaving the gate %d\n", bee_id, gate_id);

        message bee_confirmation_out_message = {leave_confirmation_type(gate_id), 0};
        if (msgsnd(gate_message_queue, &bee_confirmation_out_message, sizeof(int), 0) == -1)
        {
            printf("error sending leave confirmation\n");
            return 1;
        }
        current_state = STATE_OUTSIDE;
        printf("Bee %d is outside\n", bee_id);

        sleep(rand() % 10); // TODO: this should be read from the command line arguments
    }

    printf("bee %d is dead\n", bee_id);

    return 0;
}