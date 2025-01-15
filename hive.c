#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <dispatch/dispatch.h>
#include <stdio.h>

#define microseconds_to_milliseconds(x) (x * 1000)
#define with_fuzz(x) (x + (rand() % x))

#define ENTER_REQUEST_TYPE 1
#define LEAVE_REQUEST_TYPE 2
#define enter_confirmation_type(gate_id) (3 + gate_id)
#define leave_confirmation_type(gate_id) (3 + GATES_NUMBER + gate_id)
#define allow_use_gate_type(bee_id) (3 + 2 * GATES_NUMBER + bee_id)

#define GATES_NUMBER 2

int gate_message_queue;

void handle_sigint(int sig) {
    printf("\nCaught Ctrl+C (SIGINT signal). Performing cleanup...\n");
    cleanup_resources();
    printf("Resources cleaned up. Exiting program.\n");
    exit(0);
}

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

pthread_mutex_t bees_inside_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
dispatch_semaphore_t bees_inside_counter_semaphore;
int bees_inside_counter = 0;

pthread_mutex_t gates_mutex[GATES_NUMBER];
typedef struct gate_thread_args
{
    int gate_id;
} gate_thread_args;

pthread_t gate_enter_threads[GATES_NUMBER];
pthread_t gate_leave_threads[GATES_NUMBER];
gate_thread_args gate_args[GATES_NUMBER];

void *gate_leave_thread(void *arg)
{
    gate_thread_args *args = (gate_thread_args *)arg;
    int gate_id = args->gate_id;

    while (1)
    {

        printf("Gate %d: waiting for leave request\n", gate_id);
        message bee_request_out;
        if (msgrcv(gate_message_queue, &bee_request_out, sizeof(int), LEAVE_REQUEST_TYPE, 0) == -1)
        {
            cleanup_resources();
            exit(1);
        }

        pthread_mutex_lock(&gates_mutex[gate_id]);

        printf("Gate %d: received leave request from bee %d\n", gate_id, bee_request_out.data);
        int bee_id = bee_request_out.data;
        message bee_allow_out_message = {allow_use_gate_type(bee_id), 0};
        if (msgsnd(gate_message_queue, &bee_allow_out_message, sizeof(int), 0) == -1)
        {
            cleanup_resources();
            exit(2);
        }

        printf("Gate %d: allowed bee %d to leave\n", gate_id, bee_id);
        message bee_confirmation_out_message;
        if (msgrcv(gate_message_queue, &bee_confirmation_out_message, sizeof(int), leave_confirmation_type(gate_id), 0) == -1)
        {
            cleanup_resources();
            exit(3);
        }

        printf("Gate %d: bee %d left\n", gate_id, bee_id);
        pthread_mutex_lock(&bees_inside_counter_mutex);
        bees_inside_counter--;
        pthread_mutex_unlock(&bees_inside_counter_mutex);

        dispatch_semaphore_signal(bees_inside_counter_semaphore);

        pthread_mutex_unlock(&gates_mutex[gate_id]);
    }

    return NULL;
}

void *gate_enter_thread(void *args)
{
    gate_thread_args *gate_args = (gate_thread_args *)args;
    int gate_id = gate_args->gate_id;

    while (1)
    {
        message bee_request_in;
        printf("Gate %d: waiting for enter request\n", gate_id);
        dispatch_semaphore_wait(bees_inside_counter_semaphore, DISPATCH_TIME_FOREVER);
        printf("Gate %d: waiting for enter request after semapthore\n", gate_id);
        if (msgrcv(gate_message_queue, &bee_request_in, sizeof(int), ENTER_REQUEST_TYPE, 0) == -1)
        {
            cleanup_resources();
            exit(4);
        }

        pthread_mutex_lock(&gates_mutex[gate_id]);

        int bee_id = bee_request_in.data;
        printf("Gate %d: received enter request from bee %d\n", gate_id, bee_id);
        message bee_allow_in_message = {allow_use_gate_type(bee_id), gate_id};
        if (msgsnd(gate_message_queue, &bee_allow_in_message, sizeof(int), 0) == -1)
        {
            cleanup_resources();
            exit(5);
        }

        printf("Gate %d: allowed bee %d to enter\n", gate_id, bee_id);
        message bee_confirmation_in_message;
        if (msgrcv(gate_message_queue, &bee_confirmation_in_message, sizeof(int), enter_confirmation_type(gate_id), 0) == -1)
        {
            cleanup_resources();
            exit(6);
        }

        printf("Gate %d: bee %d entered\n", gate_id, bee_id);
        pthread_mutex_lock(&bees_inside_counter_mutex);
        bees_inside_counter++;
        pthread_mutex_unlock(&bees_inside_counter_mutex);

        pthread_mutex_unlock(&gates_mutex[gate_id]);
    }

    return NULL;
}

void initialize_gate_message_queue()
{
    key_t key = ftok("worker.c", 65);
    gate_message_queue = msgget(key, 0666 | IPC_CREAT);
}

void register_signal_handlers()
{
    signal(SIGINT, handle_sigint);
}

void initialize_gate_synchronization_mechanisms() {
    bees_inside_counter_semaphore = dispatch_semaphore_create(10); // TODO: number of max bees in the hive - bees that are currently in the hive.
    for (int i = 0; i < GATES_NUMBER; i++)
    {
        pthread_mutex_init(&gates_mutex[i], NULL);
    }
}

void cleanup_synchronization_mechanisms() {
    for (int i = 0; i < GATES_NUMBER; i++)
    {
        pthread_mutex_destroy(&gates_mutex[i]);
    }
}

void cleanup_resources() {
    msgctl(gate_message_queue, IPC_RMID, NULL);
    cleanup_synchronization_mechanisms();
}

void initialize_gate_threads()
{
    for (int i = 0; i < GATES_NUMBER; i++)
    {
        gate_args[i].gate_id = i;
        pthread_create(&gate_enter_threads[i], NULL, gate_enter_thread, &gate_args[i]);
        pthread_create(&gate_leave_threads[i], NULL, gate_leave_thread, &gate_args[i]);
    }
}

void join_gate_threads()
{
    for (int i = 0; i < GATES_NUMBER; i++)
    {
        pthread_join(gate_enter_threads[i], NULL);
        pthread_join(gate_leave_threads[i], NULL);
    }
}

int main()
{
    register_signal_handlers();
    initialize_gate_message_queue();
    initialize_gate_synchronization_mechanisms();
    initialize_gate_threads();
    join_gate_threads();
    cleanup_resources();

    return 0;
}
