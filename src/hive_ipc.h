
#include <semaphore.h>

#ifndef HIVE_IPC_H
#define HIVE_IPC_H

#define USED_GATE_TYPE 1
#define ACK_TYPE 2
#define GIVE_BIRTH 3
#define GATES_NUMBER 2

typedef struct
{
    long type;
    int delta;
} gate_message;

typedef struct
{
    long type;
    int data;
} queen_message;


int queen_message_queue;
int gate_message_queue[GATES_NUMBER];
sem_t *gate_semaphore[GATES_NUMBER];
sem_t *room_inside_semaphore;

int open_semaphores(int);

int initialize_gate_message_queue();

int initialize_queen_message_queue();

void close_semaphores();

void unlink_semaphores();

void close_gate_message_queue();

void close_queen_message_queue();

#endif