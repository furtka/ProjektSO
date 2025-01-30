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
#include <errno.h>

#include "logger/logger.h"
#include "hive_ipc.h"

#define log_tag "HIVE"

#define max(a, b) a > b ? a : b;

/**
 * Represents the configuration of a bee.
 */
typedef struct
{
    int id;
    int time_in_hive;
    int life_span;
    int starts_in_hive;
} bee_config;

int child_pid_group = -1;

#define handle_error(x)                                                                               \
    if (!sigint && x == -1)                                                                                      \
    {                                                                                                 \
        log(LOG_LEVEL_ERROR, log_tag, "ERROR %s at %s at %d\n", strerror(errno), __func__, __LINE__); \
        if (!sigint) try_clean_and_exit_with_error();                                                              \
    }

volatile sig_atomic_t sigint = 0;

int max_bees_capacity;
char *bees_config_filepath;
char *logs_directory;
int next_bee_id = 0;

void try_clean_and_exit_with_error();
void try_clean_and_exit();
void cleanup_resources();
void initialize_gate_threads();
void launch_bee_process(bee_config bee);

pthread_mutex_t bees_inside_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
int bees_inside_counter = 0;

pthread_t gate_threads[GATES_NUMBER];
pthread_t zombie_collector_thread;
pthread_t queen_thread;

int wait_for_child()
{
    printf("Waiting for child\n");
    int status = 0;
    if (waitpid(-1, &status, 0) == -1)
    {
        if (errno == ECHILD)
        {
            return 1;
        }
        else
        {
            log(LOG_LEVEL_ERROR, log_tag, "ERROR %s at %s\n", strerror(errno), __func__);
            if (!sigint)
            {
                try_clean_and_exit_with_error();
            }
            return 1;
        }
    }
    else
    {
        if (!sigint && WEXITSTATUS(status) != 0)
        {
            try_clean_and_exit_with_error();
        }
        return 1;
    }

    return 0;
}

void *zombie_collector_thread_function(void *arg)
{
    while (!sigint)
    {
        if (wait_for_child() == 1)
        {
            sleep(1);
        }
    }
    return NULL;
}

void *gate_thread_function(void *arg)
{
    int gate_id = *((int *)arg);
    free(arg);
    while (!sigint)
    {
        gate_message message;
        handle_error(msgrcv(gate_message_queue[gate_id], &message, sizeof(int), USED_GATE_TYPE, 0));
        log(LOG_LEVEL_DEBUG, log_tag, "Gate %d: Received message", gate_id);
        pthread_mutex_lock(&bees_inside_counter_mutex);
        bees_inside_counter += message.delta;
        log(LOG_LEVEL_DEBUG, log_tag, "Gate %d: %d bees inside", gate_id, bees_inside_counter);
        pthread_mutex_unlock(&bees_inside_counter_mutex);
        message.type = ACK_TYPE;
        log(LOG_LEVEL_DEBUG, log_tag, "Gate %d: Acknowledging", gate_id);
        handle_error(msgsnd(gate_message_queue[gate_id], &message, sizeof(int), 0));
    }
    return NULL;
}

void *queen_thread_function(void *arg)
{
    while (!sigint)
    {
        queen_message message;
        log(LOG_LEVEL_INFO, log_tag, "Awaiting message from queen");
        if (!sigint)
            handle_error(msgrcv(queen_message_queue, &message, sizeof(int), GIVE_BIRTH, 0));
        log(LOG_LEVEL_INFO, log_tag, "Recieved message from queen, creating new bee");

        if (!sigint)
        {
            launch_bee_process(
                (bee_config){
                    .id = next_bee_id++,
                    .time_in_hive = rand() % 10 + 2,
                    .life_span = rand() % 10 + 2,
                    .starts_in_hive = 1});
        }
    }
    return NULL;
}

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
}

void initialize_zombie_collector_thread()
{
    pthread_create(&zombie_collector_thread, NULL, zombie_collector_thread_function, NULL);
}

void initialize_gate_threads()
{
    for (int i = 0; i < GATES_NUMBER; i++)
    {
        int *gate_id = (int *)malloc(sizeof(int));
        *gate_id = i;
        pthread_create(&gate_threads[i], NULL, gate_thread_function, gate_id);
    }
}

void initialize_queen_thread()
{
    pthread_create(&queen_thread, NULL, queen_thread_function, NULL);
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

#define REASONABLE_INPUT_MAX_NUMBER 100
#define REASONABLE_INPUT_MIN_NUMBER 1

/**
 * Reads an integer from the file and stores it in the output parameter.
 *
 * @param file File to read from.
 * @param output Pointer to the integer where the read value will be stored.
 *
 * @return 1 if the integer was successfully read, 0 otherwise.
 */
int read_integer(FILE *file, int *output)
{
    char buffer[100];
    if (fscanf(file, "%99s", buffer) != 1)
    {
        return 0;
    }

    char *endptr;
    errno = 0;
    long value = strtol(buffer, &endptr, 10);

    if (errno == ERANGE || value > REASONABLE_INPUT_MAX_NUMBER || value < REASONABLE_INPUT_MIN_NUMBER || *endptr != '\0')
    {
        return 0;
    }

    *output = (int)value;
    return 1;
}

/**
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
 *
 * All the numbers should be in reasonable range.
 * Reasonable means in range [1 100]
 */
hive_config read_config_file()
{
    FILE *config_file = fopen(bees_config_filepath, "r");
    if (!config_file)
    {
        fprintf(stderr, "Error opening config file\n");
        exit(1);
    }

    int new_bee_interval, number_of_bees, max_bees_capacity;

    if (!read_integer(config_file, &number_of_bees) || number_of_bees <= 0)
    {
        fprintf(stderr, "Invalid number of bees (N)\n");
        fclose(config_file);
        exit(1);
    }

    if (!read_integer(config_file, &max_bees_capacity) || max_bees_capacity <= 0)
    {
        fprintf(stderr, "Invalid maximum hive capacity (P)\n");
        fclose(config_file);
        exit(1);
    }

    if (!read_integer(config_file, &new_bee_interval) || new_bee_interval < 0)
    {
        fprintf(stderr, "Invalid new bee interval (T)\n");
        fclose(config_file);
        exit(1);
    }

    if (number_of_bees < 2 * max_bees_capacity)
    {
        fprintf(stderr, "Invalid number of bees and maximum hive capacity\n");
        fclose(config_file);
        exit(1);
    }

    int *bee_time_in_hive = (int *)malloc(number_of_bees * sizeof(int));
    int *bee_life_spans = (int *)malloc(number_of_bees * sizeof(int));

    if (!bee_time_in_hive || !bee_life_spans)
    {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(config_file);
        exit(1);
    }

    for (int i = 0; i < number_of_bees; i++)
    {
        if (!read_integer(config_file, &bee_time_in_hive[i]) || bee_time_in_hive[i] < 0)
        {
            fprintf(stderr, "Invalid time in hive (T_%d)\n", i + 1);
            free(bee_time_in_hive);
            free(bee_life_spans);
            fclose(config_file);
            exit(1);
        }
    }

    for (int i = 0; i < number_of_bees; i++)
    {
        if (!read_integer(config_file, &bee_life_spans[i]) || bee_life_spans[i] < 0)
        {
            fprintf(stderr, "Invalid life span (X_%d)\n", i + 1);
            free(bee_time_in_hive);
            free(bee_life_spans);
            fclose(config_file);
            exit(1);
        }
    }

    fclose(config_file);

    hive_config config = {
        .max_bees_capacity = max_bees_capacity,
        .number_of_bees = number_of_bees,
        .new_bee_interval = new_bee_interval,
        .bees = (bee_config *)malloc(number_of_bees * sizeof(bee_config))};

    if (!config.bees)
    {
        fprintf(stderr, "Memory allocation failed for bees\n");
        free(bee_time_in_hive);
        free(bee_life_spans);
        exit(1);
    }

    for (int i = 0; i < number_of_bees; i++)
    {
        config.bees[i].id = i;
        config.bees[i].time_in_hive = bee_time_in_hive[i];
        config.bees[i].life_span = bee_life_spans[i];
        config.bees[i].starts_in_hive = rand() % 2;

        next_bee_id = max(next_bee_id, i);
    }

    free(bee_time_in_hive);
    free(bee_life_spans);
    next_bee_id++;
    return config;
}

void launch_bee_process(bee_config bee)
{
    char *id;
    char *life_span;
    char *time_in_hive;

    pid_t pid = fork();
    switch (pid)
    {
    case -1:
        log(LOG_LEVEL_ERROR, log_tag, "Error launching bee process, exiting...");
        try_clean_and_exit_with_error();
        break;
    case 0:
        id = (char *)malloc(6);
        sprintf(id, "%d", bee.id + 1);
        life_span = (char *)malloc(6);
        sprintf(life_span, "%d", bee.life_span);
        time_in_hive = (char *)malloc(6);
        sprintf(time_in_hive, "%d", bee.time_in_hive);
        execl("./bin/bee", "./bin/bee", id, life_span, time_in_hive, time_in_hive, "0", NULL);
        log(LOG_LEVEL_ERROR, "HIVE", "Error launching bee process, exiting...");
        try_clean_and_exit_with_error();
        break;
    default:
        if (child_pid_group == -1)
        {
            child_pid_group = pid;
        }
        setpgid(pid, child_pid_group);
        break;
    }
}

void launch_queen_process(int new_bee_interval)
{
    char *interval;
    pid_t pid = fork();
    switch (pid)
    {
    case -1:
        log(LOG_LEVEL_ERROR, log_tag, "Error launching queen process, exiting...");
        try_clean_and_exit_with_error();
        break;
    case 0:
        interval = (char *)malloc(10);
        sprintf(interval, "%d", new_bee_interval);
        execl("./bin/queen", "./bin/queen", interval, NULL);
        log(LOG_LEVEL_ERROR, "HIVE", "Error launching queen process, exiting...");
        try_clean_and_exit_with_error();
        break;
    default:
        if (child_pid_group == -1)
        {
            child_pid_group = pid;
        }
        setpgid(pid, child_pid_group);
        break;
    }
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
        launch_bee_process(config.bees[i]);
    }
}

void cleanup_resources()
{
    printf("cleanup resources called\n");
    kill(-child_pid_group, SIGINT);
    sigint = 1;
    while (wait_for_child() == 0)
        ;

    close_gate_message_queue();
    close_queen_message_queue();
    close_semaphores();
    unlink_semaphores();
    close_logger();
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

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);
    init_logger();
    log(LOG_LEVEL_INFO, "HIVE", "Starting hive");
    parse_command_line_arguments(argc, argv);
    hive_config config = read_config_file();
    handle_error(initialize_gate_message_queue());
    handle_error(initialize_queen_message_queue());
    handle_error(open_semaphores(config.max_bees_capacity));
    launch_bee_processes(config);
    launch_queen_process(config.new_bee_interval);

    initialize_queen_thread();
    initialize_zombie_collector_thread();
    initialize_gate_threads();

    while (!sigint)
    {
        sleep(1);
    }

    try_clean_and_exit();
}
