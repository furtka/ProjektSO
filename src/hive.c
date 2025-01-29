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
#include <sys/stat.h>

#include "logger/logger.h"
#include "hive_ipc.h"

typedef struct
{
    int *array;
    size_t used;
    size_t size;
} vector;

void init_vector(vector *v, size_t init_size)
{
    v->array = malloc(init_size * sizeof(int));
    v->used = 0;
    v->size = init_size;
}

void push_back(vector *v, int new_element)
{
    if (v->used == v->size)
    {
        v->size *= 2;
        v->array = realloc(v->array, v->size * sizeof(int));
    }
    v->array[v->used++] = new_element;
}

void free_vector(vector *v)
{
    free(v->array);
    v->array = NULL;
    v->used = v->size = 0;
}

int get(vector *v, int index)
{
    return v->array[index];
}

int max_bees_capacity;
char *bees_config_filepath;
vector *children_processes;
char *logs_directory;

void cleanup_resources();
void handle_sigint(int);
void register_signal_handlers();
void initialize_gate_synchronization_mechanisms();
void cleanup_synchronization_mechanisms();
void initialize_gate_threads();
void join_gate_threads();
void propagate_sigint_to_children();

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
pthread_t queen_handle_thread;
gate_thread_args gate_args[GATES_NUMBER];

RESULT handle_gate_leave_request(int gate_id)
{
    log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for leave request on gate %d", gate_id);
    handle_result_as(bee_id, await_bee_request_leave());

    log(LOG_LEVEL_DEBUG, "HIVE", "Received leave request from bee %d on gate %d and waiting for lock", bee_id, gate_id);
    pthread_mutex_lock(&gates_mutex[gate_id]);
    log(LOG_LEVEL_DEBUG, "HIVE", "Sending allow leave message to bee %d on gate %d", bee_id, gate_id);
    handle_failure(send_bee_allow_leave_message(bee_id, gate_id));

    log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for leave confirmation from bee %d on gate %d", bee_id, gate_id);
    handle_failure(await_bee_leave_confirmation(gate_id));

    log(LOG_LEVEL_DEBUG, "HIVE", "Bee %d left through gate %d, waiting for lock to decrement the counter", bee_id, gate_id);
    pthread_mutex_lock(&bees_inside_counter_mutex);
    bees_inside_counter--;
    log(LOG_LEVEL_DEBUG, "HIVE", "Bees inside: %d", bees_inside_counter);
    pthread_mutex_unlock(&bees_inside_counter_mutex);

    dispatch_semaphore_signal(bees_inside_counter_semaphore);

    pthread_mutex_unlock(&gates_mutex[gate_id]);
    log(LOG_LEVEL_DEBUG, "HIVE", "Unlocking gate %d", gate_id);

    return SUCCESS;
}

RESULT handle_gate_enter_request(int gate_id)
{
    log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for semaphore on gate %d", gate_id);
    dispatch_semaphore_wait(bees_inside_counter_semaphore, DISPATCH_TIME_FOREVER);
    log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for enter request on gate %d", gate_id);

    handle_result_as(bee_id, await_bee_request_enter());
    log(LOG_LEVEL_DEBUG, "HIVE", "Received enter request from bee %d on gate %d and waiting for lock", bee_id, gate_id);
    pthread_mutex_lock(&gates_mutex[gate_id]);

    log(LOG_LEVEL_DEBUG, "HIVE", "Sending allow enter message to bee %d on gate %d", bee_id, gate_id);
    handle_failure(send_bee_allow_enter_message(bee_id, gate_id));

    log(LOG_LEVEL_DEBUG, "HIVE", "Waiting for enter confirmation from bee %d on gate %d", bee_id, gate_id);
    handle_failure(await_bee_enter_confirmation(gate_id));

    log(LOG_LEVEL_DEBUG, "HIVE", "Bee %d entered through gate %d, waiting for lock to increment the counter", bee_id, gate_id);
    pthread_mutex_lock(&bees_inside_counter_mutex);
    bees_inside_counter++;
    log(LOG_LEVEL_DEBUG, "HIVE", "Bees inside: %d, counter is incremented", bees_inside_counter);
    pthread_mutex_unlock(&bees_inside_counter_mutex);

    log(LOG_LEVEL_DEBUG, "HIVE", "Unlocking gate %d", gate_id);
    pthread_mutex_unlock(&gates_mutex[gate_id]);

    return SUCCESS;
}

RESULT handle_queen_birth_request()
{
    log(LOG_LEVEL_INFO, "HIVE", "Awaiting queen birth request");
    handle_failure(await_queen_birth_request());

    log(LOG_LEVEL_INFO, "HIVE", "Waiting for semaphore for handle queen birth request");
    dispatch_semaphore_wait(bees_inside_counter_semaphore, DISPATCH_TIME_FOREVER);
    log(LOG_LEVEL_INFO, "HIVE", "Sending queen birth allowance");
    handle_failure(send_queen_birth_allowance());

    log(LOG_LEVEL_INFO, "HIVE", "Waiting for queen birth confirmation");
    handle_result_as(new_bee_pid, await_queen_birth_confirmation());

    push_back(children_processes, new_bee_pid);
    pthread_mutex_lock(&bees_inside_counter_mutex);
    bees_inside_counter++;
    log(LOG_LEVEL_DEBUG, "HIVE", "Bees inside: %d, counter is incremented after birth", bees_inside_counter);
    pthread_mutex_unlock(&bees_inside_counter_mutex);

    return SUCCESS;
}

void *gate_leave_thread(void *arg)
{
    gate_thread_args *args = (gate_thread_args *)arg;
    RESULT result;
    while ((result = handle_gate_leave_request(args->gate_id)) == SUCCESS)
        ;

    if (result == FAILURE)
    {
        log(LOG_LEVEL_ERROR, "HIVE", "Error handling gate leave request");
    }

    return NULL;
}

void *gate_enter_thread(void *args)
{
    gate_thread_args *gate_args = (gate_thread_args *)args;
    int gate_id = gate_args->gate_id;

    RESULT result;
    while ((result = handle_gate_enter_request(gate_id)) == SUCCESS)
        ;

    if (result == FAILURE)
    {
        log(LOG_LEVEL_ERROR, "HIVE", "Error handling gate enter request");
    }

    return NULL;
}

void *queen_handle_thread_function(void *args)
{
    RESULT result;
    while ((result = handle_queen_birth_request()) == SUCCESS)
        ;

    if (result == FAILURE)
    {
        log(LOG_LEVEL_ERROR, "HIVE", "Error handling queen birth request");
    }

    return NULL;
}

void cleanup_resources()
{
    log(LOG_LEVEL_INFO, "HIVE", "Cleaning up resources");
    cleanup_gate_message_queue();
    cleanup_synchronization_mechanisms();
    close_logger();
    free_vector(children_processes);
    free(children_processes);
}

/**
 * Propagates the SIGINT signal to all child processes.
 */
void propagate_sigint_to_children()
{
    for (int i = 0; i < children_processes->used; i++)
    {
        kill(get(children_processes, i), SIGINT);
    }
}

volatile sig_atomic_t sigint = 0;
/**
 * Handles the SIGINT by propagating it to all child processes.
 *
 * If the signal is not SIGINT, it does nothing.
 *
 * @param singal Signal number
 */
void handle_sigint(int singal)
{
    sigint = 1;
    propagate_sigint_to_children();
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

void initialize_queen_handle_thread()
{
    pthread_create(&queen_handle_thread, NULL, queen_handle_thread_function, NULL);
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
 * Represents the configuration of a bee.
 */
typedef struct
{
    int id;
    int time_in_hive;
    int life_span;
} bee_config;

/**
 * Represents the configuration of the hive, including the bees and queen.
 *
 */
typedef struct
{
    int max_bees_capacity;
    int number_of_bees;
    int new_bee_interval;
    bee_config *bees;
} hive_config;

/**
 * TODO: Add input verification
 *
 * Reads the configuration file and returns the hive configuration.
 *
 * The expected file format is:
 * <config>
 * N P
 * T
 * T_1 T_2 ... T_N
 * X_1 X_2 ... X_N
 * </config>
 *
 * Where:
 *  N is the number of bees
 *  P is the maximum number of bees in the hive
 *  T is the interval for new bees to be created by the queen
 *  T_i is the time that the i-th bee spends in the hive
 *  X_i is the life span of the i-th bee in terms of times it leaves the hive
 */
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
    fscanf(config_file, "%d %d %d", &number_of_bees, &max_bees_capacity, &new_bee_interval);

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

/**
 * Launches the bee processes and returns the pids of the processes.
 *
 * @param config - config of the hive
 */
void launch_bee_processes(hive_config config)
{
    for (int i = 0; i < config.number_of_bees; i++)
    {
        bee_config bee = config.bees[i];
        int pid = fork();
        if (pid == 0)
        {
            char *id = (char *)malloc(6);
            sprintf(id, "%d", bee.id + 1);
            char *life_span = (char *)malloc(6);
            sprintf(life_span, "%d", bee.life_span);
            char *time_in_hive = (char *)malloc(6);
            sprintf(time_in_hive, "%d", bee.time_in_hive);

            execl("./bin/bee", "./bin/bee", id, life_span, time_in_hive, time_in_hive, "0", NULL);
            log(LOG_LEVEL_ERROR, "HIVE", "Error launching bee process, exiting...");
            propagate_sigint_to_children();
            cleanup_resources();
            exit(1);
        }
        push_back(children_processes, pid);
    }
}

void launch_queen_process(int new_bee_interval, int next_bee_id)
{
    int pid = fork();
    push_back(children_processes, pid);
    if (pid == 0)
    {
        char *new_bee_interval_str = (char *)malloc(12);
        sprintf(new_bee_interval_str, "%d", new_bee_interval);
        char *next_bee_id_str = (char *)malloc(12);
        sprintf(next_bee_id_str, "%d", next_bee_id);

        execl("./bin/queen", "./bin/queen", new_bee_interval_str, next_bee_id_str, NULL);
        log(LOG_LEVEL_ERROR, "HIVE", "Error launching queen process, exiting...");
        propagate_sigint_to_children();
        cleanup_resources();
        exit(1);
    }
    push_back(children_processes, pid);
}

void wait_for_children_processes()
{
    printf("waiting for children\n");
    printf("children_processes->used: %d\n", children_processes->used);
    for (int i = 0; i < children_processes->used; i++)
    {
        printf("waiting for index: child_processes[%d] = %d on %d\n", i, get(children_processes, i), children_processes->used);
        waitpid(get(children_processes, i), NULL, 0);
    }
}

int main(int argc, char *argv[])
{
    children_processes = malloc(sizeof(vector));
    init_vector(children_processes, 10);
    init_logger();
    log(LOG_LEVEL_INFO, "HIVE", "Starting hive");
    parse_command_line_arguments(argc, argv);
    hive_config config = read_config_file();
    launch_bee_processes(config);
    launch_queen_process(config.new_bee_interval, config.number_of_bees + 10);

    register_signal_handlers();
    initialize_gate_message_queue();
    initialize_gate_synchronization_mechanisms();
    initialize_gate_threads();
    initialize_queen_handle_thread();

    while (!sigint)
    {
        sleep(1);
    }

    wait_for_children_processes();
    cleanup_resources();
    return 0;
}
