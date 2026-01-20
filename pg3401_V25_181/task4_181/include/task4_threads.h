#ifndef TASK4_THREADS_H
#define TASK4_THREADS_H

#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define BUFFER_SIZE 4096
// sem_empty: slot available for producer (1 = empty)
// sem_full: slot available for consumer (1 = full)
// sem_mutex: Ensure that buffer is ready to write
typedef struct {
    unsigned char   buffer[BUFFER_SIZE];
    int             bytes_in_buffer;
    bool            producer_done;
    sem_t           sem_empty;
    sem_t           sem_full;
    sem_t           sem_mutex;
    const char     *infile;
    FILE           *outfp;
    const uint32_t *key;
} ctx_t;

void *thread_producer(void *arg);
void *thread_consumer(void *arg);

#endif