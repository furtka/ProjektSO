#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <dispatch/dispatch.h>
#include <stdio.h>
#include <signal.h>

#include "logger.h"
#include "hive_ipc.h"

int max_bees_capacity;
char *bees_config_filepath;
int* pids;

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
        log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for leave request on gate %d", gate_id);
        int bee_id = await_bee_request_leave();
        if (bee_id < 0)
        {
            cleanup_resources();
            exit(1);
        }

        log(LOG_LEVEL_DEBUG, "HIVE", "Received leave request from bee %d on gate %d and waiting for lock", bee_id, gate_id);
        pthread_mutex_lock(&gates_mutex[gate_id]);
        log(LOG_LEVEL_DEBUG, "HIVE", "Sending allow leave message to bee %d on gate %d", bee_id, gate_id);
        if (send_bee_allow_leave_message(bee_id, gate_id) == FAILURE)
        {
            cleanup_resources();
            exit(2);
        }

        log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for leave confirmation from bee %d on gate %d", bee_id, gate_id);
        if (await_bee_leave_confirmation(gate_id) == FAILURE)
        {
            cleanup_resources();
            exit(3);
        }

        log(LOG_LEVEL_DEBUG, "HIVE", "Bee %d left through gate %d, waiting for lock to decrement the counter", bee_id, gate_id);
        pthread_mutex_lock(&bees_inside_counter_mutex);
        bees_inside_counter--;
        log(LOG_LEVEL_DEBUG, "HIVE", "Bees inside: %d", bees_inside_counter);
        pthread_mutex_unlock(&bees_inside_counter_mutex);

        dispatch_semaphore_signal(bees_inside_counter_semaphore);

        pthread_mutex_unlock(&gates_mutex[gate_id]);
        log(LOG_LEVEL_DEBUG, "HIVE", "Unlocking gate %d", gate_id);
    }

    return NULL;
}

void *gate_enter_thread(void *args)
{
    gate_thread_args *gate_args = (gate_thread_args *)args;
    int gate_id = gate_args->gate_id;

    while (1)
    {
        log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for semaphore on gate %d", gate_id);
        dispatch_semaphore_wait(bees_inside_counter_semaphore, DISPATCH_TIME_FOREVER);
        log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for enter request on gate %d", gate_id);
        int bee_id = await_bee_request_enter();
        if (bee_id < 0)
        {
            cleanup_resources();
            exit(1);
        }
        log(LOG_LEVEL_DEBUG, "HIVE", "Received enter request from bee %d on gate %d and waiting for lock", bee_id, gate_id);
        pthread_mutex_lock(&gates_mutex[gate_id]);

        log(LOG_LEVEL_DEBUG, "HIVE", "Sending allow enter message to bee %d on gate %d", bee_id, gate_id);
        if (send_bee_allow_enter_message(bee_id, gate_id) == FAILURE)
        {
            cleanup_resources();
            exit(4);
        }

        log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for enter confirmation from bee %d on gate %d", bee_id, gate_id);
        if (await_bee_enter_confirmation(gate_id) == FAILURE)
        {
            cleanup_resources();
            exit(5);
        }

        log(LOG_LEVEL_DEBUG, "HIVE", "Bee %d entered through gate %d, waiting for lock to increment the counter", bee_id, gate_id);
        pthread_mutex_lock(&bees_inside_counter_mutex);
        bees_inside_counter++;
        log(LOG_LEVEL_DEBUG, "HIVE", "Bees inside: %d, counter is incremented", bees_inside_counter);
        pthread_mutex_unlock(&bees_inside_counter_mutex);

        log(LOG_LEVEL_DEBUG, "HIVE", "Unlocking gate %d", gate_id);
        pthread_mutex_unlock(&gates_mutex[gate_id]);
    }

    return NULL;
}

void cleanup_resources()
{
    cleanup_gate_message_queue();
    cleanup_synchronization_mechanisms();

    for (int i = 0; pids[i] != 0; i++)
    {
        kill(pids[i], SIGINT);
    }
}

void handle_sigint(int singal)
{
    log(LOG_LEVEL_DEBUG, "HIVE", "Caught (SIGINT signal %d). Performing cleanup...", singal);
    cleanup_resources();
    log(LOG_LEVEL_DEBUG, "HIVE", "Resources cleaned up. Exiting program.");
    exit(0);
}

void register_signal_handlers()
{
    signal(SIGINT, handle_sigint);
}

void initialize_gate_synchronization_mechanisms()
{
    bees_inside_counter_semaphore = dispatch_semaphore_create(max_bees_capacity + 1); // TODO: number of max bees in the hive - bees that are currently in the hive.
    for (int i = 0; i < GATES_NUMBER; i++)
    {
        pthread_mutex_init(&gates_mutex[i], NULL);
    }
}

void cleanup_synchronization_mechanisms()
{
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

void parse_command_line_arguments(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <bees_config_file>\n", argv[0]);
        exit(1);
    }
    bees_config_filepath = argv[1];
}

/**
 * The config file layout:
 * N P
 * T
 * T_1 T_2 ... T_N
 * X_1 X_2 ... X_N
 * ---
 */

typedef struct
{
    int id;
    int time_in_hive;
    int life_span;
} bee_config;

typedef struct
{
    int max_bees_capacity;
    int number_of_bees;
    int new_bee_interval;
    bee_config *bees;
} hive_config;

hive_config read_config_file()
{
    FILE *config_file = fopen(bees_config_filepath, "r");
    if (!config_file)
    {
        fprintf(stderr, "Error opening config file\n");
        exit(1);
    }

    int new_bee_interval = 0;
    int number_of_bees = 0;
    fscanf(config_file, "%d %d %d", &max_bees_capacity, &number_of_bees, &new_bee_interval);

    int *bee_time_in_hive = (int *)malloc(number_of_bees * sizeof(int));
    int *bee_life_spans = (int *)malloc(number_of_bees * sizeof(int));

    for (int i = 0; i < number_of_bees; i++)
    {
        fscanf(config_file, "%d", &bee_time_in_hive[i]);
    }

    for (int i = 0; i < number_of_bees; i++)
    {
        fscanf(config_file, "%d", &bee_life_spans[i]);
    }

    fclose(config_file);

    hive_config config = {
        .max_bees_capacity = max_bees_capacity,
        .number_of_bees = number_of_bees,
        .new_bee_interval = new_bee_interval,
        .bees = (bee_config *)malloc(number_of_bees * sizeof(bee_config))};

    for (int i = 0; i < number_of_bees; i++)
    {
        config.bees[i].id = i;
        config.bees[i].time_in_hive = bee_time_in_hive[i];
        config.bees[i].life_span = bee_life_spans[i];
    }

    free(bee_time_in_hive);
    free(bee_life_spans);

    return config;
}

int *launch_bee_processes(hive_config config)
{
    int *bee_pids = (int *)malloc(config.number_of_bees * sizeof(int) + 1);
    for (int i = 0; i < config.number_of_bees; i++)
    {
        bee_config bee = config.bees[i];
        int pid = fork();
        if (pid == 0)
        {
            char *id = (char *)malloc(6);
            sprintf(id, "%d", bee.id);
            char *life_span = (char *)malloc(6);
            sprintf(life_span, "%d", bee.life_span);
            char *time_in_hive = (char *)malloc(6);
            sprintf(time_in_hive, "%d", bee.time_in_hive);

            // FIXME(furtak): bee binary might be in a different location
            execl("./bin/bee", "./bin/bee", id, life_span, time_in_hive, time_in_hive, NULL);
            printf("Error launching bee process\n");
            exit(1);
        }
        bee_pids[i] = pid;
    }

    bee_pids[config.number_of_bees] = 0;

    return bee_pids;
}

int main(int argc, char *argv[])
{
    parse_command_line_arguments(argc, argv);
    hive_config config = read_config_file();
    pids = launch_bee_processes(config);

    register_signal_handlers();
    initialize_gate_message_queue();
    initialize_gate_synchronization_mechanisms();
    initialize_gate_threads();

    for (int i = 0; pids[i] != 0; i++)
    {
        waitpid(pids[i], NULL, 0);
    }

    cleanup_resources();
    return 0;
}
