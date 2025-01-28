#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "logger_internal.h"

#define SHARED_MEMORY_NAME "/myshm"
#define SEMAPHORE_WRITE "/semaphore_write"
#define SEMAPHORE_READ "/semaphore_read"
#define WRITE_SEMAPHORE_FULL "write_semaphore_full"

sem_t *write_semaphore;
sem_t *read_semaphore;
sem_t *write_semaphore_full;
void *header;
int shmfd;

#define MAX_LOGS 100
#define MEMORY_SIZE MAX_LOGS * sizeof(LogMessage) + sizeof(Header)

void allocate()
{
    write_semaphore = sem_open(SEMAPHORE_WRITE, O_CREAT, 0644, 1);
    write_semaphore_full = sem_open(WRITE_SEMAPHORE_FULL, O_CREAT, 0644, MAX_LOGS);
    if (write_semaphore == SEM_FAILED)
    {
        perror("sem_open write_semaphore");
        deallocate_client();
        return;
    }

    read_semaphore = sem_open(SEMAPHORE_READ, O_CREAT, 0644, 0);
    if (read_semaphore == SEM_FAILED)
    {
        perror("sem_open read_semaphore");
        deallocate_client();
        return;
    }

    sem_wait(write_semaphore);

    shmfd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (shmfd == -1)
    {
        if (errno == EEXIST)
        {
            shmfd = shm_open(SHARED_MEMORY_NAME, O_RDWR, S_IRUSR | S_IWUSR);
            if (shmfd == -1)
            {
                perror("shm_open");
                deallocate_client();
                return;
            }

            header = mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
        }
        else
        {
            perror("shm_open");
        }
    }
    else
    {
        ftruncate(shmfd, MEMORY_SIZE);
        header = mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

        if (header == NULL) 
        {
            perror("mmap");
            deallocate_server();
            return;
        }
        ((Header*)header)->write = sizeof(Header);
        ((Header*)header)->read = sizeof(Header);
    }

    sem_post(write_semaphore);
}

void write_log(LogMessage *log_message)
{
    sem_wait(write_semaphore_full);
    sem_wait(write_semaphore);
    LogMessage* write_pointer = ((char*)header + ((Header*)header)->write);
    memcpy(write_pointer, log_message, sizeof(LogMessage));
    ((Header*)header)->write += sizeof(LogMessage);
    if (((Header*)header)->write == MEMORY_SIZE)
    {
        ((Header*)header)->write = sizeof(Header);
    }
    sem_post(read_semaphore);
    sem_post(write_semaphore);
 }

LogMessage *read_log()
{   
    sem_wait(read_semaphore);
    LogMessage *log_message = malloc(sizeof(LogMessage));
    LogMessage* read_pointer = ((char*)header + ((Header*)header)->read);
    memcpy(log_message, read_pointer, sizeof(LogMessage));
    ((Header*)header)->read += sizeof(LogMessage);
    if (((Header*)header)->read == MEMORY_SIZE)
    {
        ((Header*)header)->read = sizeof(Header);
    }
    sem_post(write_semaphore_full);
    return log_message;
}

void deallocate_client()
{
    printf("DEALOCATING client\n");
    sem_close(write_semaphore);
    sem_close(read_semaphore);
    sem_close(write_semaphore_full);

    munmap(header, MEMORY_SIZE);
    close(shmfd);
 }

void deallocate_server() 
{
    printf("DEALOCATING server\n");
    sem_unlink(SEMAPHORE_WRITE);
    sem_unlink(SEMAPHORE_READ);
    sem_unlink(WRITE_SEMAPHORE_FULL);
    shm_unlink(SHARED_MEMORY_NAME);
}
