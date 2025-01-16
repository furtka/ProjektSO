#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <dispatch/dispatch.h>
#include <stdio.h>

#include "logger.h"
#include "hive_ipc.h"

void cleanup_resources();
void handle_sigint(int);
void register_signal_handlers();
void initialize_gate_synchronization_mechanisms();
void cleanup_synchronization_mechanisms();
void initialize_gate_threads();
void join_gate_threads();


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

        int bee_id = await_bee_request_leave();
        if (bee_id < 0)
        {
            cleanup_resources();
            exit(1);
        }

        pthread_mutex_lock(&gates_mutex[gate_id]);

        if (send_bee_allow_leave_message(bee_id, gate_id) == FAILURE)
        {
            cleanup_resources();
            exit(2);
        }

        if (await_bee_leave_confirmation(gate_id) == FAILURE)
        {
            cleanup_resources();
            exit(3);
        }

        pthread_mutex_lock(&bees_inside_counter_mutex);
        bees_inside_counter--;
        log(LOG_LEVEL_DEBUG, "HIVE", "Bees inside: %d", bees_inside_counter);
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
        log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for bees to enter the hive for semaphore");
        dispatch_semaphore_wait(bees_inside_counter_semaphore, DISPATCH_TIME_FOREVER);
        int bee_id = await_bee_request_enter();
        if (bee_id < 0)
        {
            cleanup_resources();
            exit(1);
        }

        pthread_mutex_lock(&gates_mutex[gate_id]);

        if (send_bee_allow_enter_message(bee_id, gate_id) == FAILURE)
        {
            cleanup_resources();
            exit(4);
        }

        if (await_bee_enter_confirmation(gate_id) == FAILURE)
        {
            cleanup_resources();
            exit(5);
        }

        pthread_mutex_lock(&bees_inside_counter_mutex);
        bees_inside_counter++;
        log(LOG_LEVEL_DEBUG, "HIVE", "Bees inside: %d", bees_inside_counter);
        pthread_mutex_unlock(&bees_inside_counter_mutex);

        pthread_mutex_unlock(&gates_mutex[gate_id]);
    }

    return NULL;
}

void cleanup_resources() {
    cleanup_gate_message_queue();
    cleanup_synchronization_mechanisms();
}

void handle_sigint(int singal) {
    log(LOG_LEVEL_DEBUG, "HIVE", "Caught (SIGINT signal %d). Performing cleanup...", singal);
    cleanup_resources();
    log(LOG_LEVEL_DEBUG, "HIVE", "Resources cleaned up. Exiting program.");
    exit(0);
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
