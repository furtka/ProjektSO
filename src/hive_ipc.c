#include "hive_ipc.h"
#include "logger/logger.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>

int open_semaphores(int max_bees)
{
    room_inside_semaphore = sem_open("/room_inside_semaphore", O_CREAT, 0666, max_bees);
    if (room_inside_semaphore == SEM_FAILED)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
        return -1;
    }
    gate_semaphore[0] = sem_open("/gate_semaphore_0", O_CREAT, 0666, 1);
    if (gate_semaphore[0] == SEM_FAILED)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
        return -1;
    }
    gate_semaphore[1] = sem_open("/gate_semaphore_1", O_CREAT, 0666, 1);
    if (gate_semaphore[1] == SEM_FAILED)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
        return -1;
    }

    return 0;
}

int initialize_gate_message_queue()
{
     key_t key_1 = ftok("hive", 65);
    message_queue[0] = msgget(key_1, IPC_CREAT | 0666);
    if (message_queue[0] == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
        return -1;
    }
    key_t key_2 = ftok("hive", 66);
    message_queue[1] = msgget(key_2, IPC_CREAT | 0666);
    if (message_queue[1] == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
        return -1;
    }
    return 0;
}

void close_semaphores()
{
    if (sem_close(room_inside_semaphore) == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
    }
    if (sem_close(gate_semaphore[0]) == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
    }
    if (sem_close(gate_semaphore[1]) == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
    }
}

void unlink_semaphores()
{
    if (sem_unlink("/room_inside_semaphore") == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
    }
    if (sem_unlink("/gate_semaphore_0") == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
    }
    if (sem_unlink("/gate_semaphore_1") == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
    }
}

void close_gate_message_queue()
{
    if (msgctl(message_queue[0], IPC_RMID, NULL) == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
    }

    if (msgctl(message_queue[1], IPC_RMID, NULL) == -1)
    {
        log(LOG_LEVEL_ERROR, "HIVE_IPC", "ERROR %s at %s\n", strerror(errno), __func__);
    }
}
