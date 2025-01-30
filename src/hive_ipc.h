
#include <semaphore.h>

#ifndef HIVE_IPC_H
#define HIVE_IPC_H

#define USED_GATE_TYPE 1
#define ACK_TYPE 2
#define GIVE_BIRTH 3
#define GATES_NUMBER 2

/**
 * Structure representing the message to coordinate the usage of the gates.
 * The delta field represents the number of bees entering or leaving the hive.
 * It is either 1 or -1.
 * 
 * Type is either USED_GATE_TYPE - sent by bee process or ACK_TYPE sent by hive.
 */
typedef struct
{
    long type;
    int delta;
} gate_message;

/**
 * Structure representing the message sent by the queen to the hive 
 */
typedef struct
{
    long type;
    int data;
} queen_message;

/**
 * global variable for the message queue used to communicate with the queen
 */
int queen_message_queue;

/**
 * global variable for the message queue used to communicate between the gates
 * and the hive
 */
int gate_message_queue[GATES_NUMBER];

/**
 * global variable for the semaphore used to control access the gates of the hive
 */
sem_t *gate_semaphore[GATES_NUMBER];

/**
 * global variable for the semaphore used to control the number of bees inside
 * the hive
 */
sem_t *room_inside_semaphore;

/**
 * Initialzes the semaphores used in the hive
 *
 * @return int - 0 if the semaphores were successfully initialized, -1 otherwise
 */
int open_semaphores(int);

/**
 * Initializes the message queue used to communicate between the gates and the hive
 *
 * @return int - 0 if the message queue was successfully initialized, -1 otherwise
 */
int initialize_gate_message_queue();

/**
 * Initializes the message queue used to communicate with the queen
 *
 * @return int - 0 if the message queue was successfully initialized, -1 otherwise
 */
int initialize_queen_message_queue();

/**
 * Closes the semaphores used in the hive
 */
void close_semaphores();

/**
 * Unlinks the semaphores used in the hive.
 * Should be used by the hive process only.
 */
void unlink_semaphores();

/**
 * Closes the message queue used to communicate between the gates and the hive
 * Should be used by the hive process only.
 */
void close_gate_message_queue();

/**
 * Closes the message queue used to communicate with the queen
 * Should be used by the hive process only.
 */
void close_queen_message_queue();

#endif