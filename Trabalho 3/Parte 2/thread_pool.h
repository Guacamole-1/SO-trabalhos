
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>  


typedef void *(*wi_function_t)(void *);

typedef struct
{
    void **buffer;
    int    enqueue;        
    int    dequeue;        
    int    nelems;         
    int    maxCapacity;    

    pthread_cond_t   is_full_c;
    pthread_cond_t   is_empty_c;

    pthread_mutex_t  mutex;

} SharedBuffer;


void sharedBuffer_init    (SharedBuffer *sb, int capacity);
void sharedBuffer_destroy (SharedBuffer *sb);
void sharedBuffer_enqueue (SharedBuffer *sb, void *data);
void *sharedBuffer_dequeue(SharedBuffer *sb);


typedef struct {
    wi_function_t fn;            
    void         *arg;          
} work_item_t;


typedef struct {
    SharedBuffer   queue;                     
    int            nthreads_min;        
    int            nthreads_max;                  
    int            nthreads_alive;  
    int            nthreads_idle;  // threads não ativas     
    pthread_t     *threads;                     
    pthread_mutex_t lock;         
    int            shutdown;                     
} threadpool_t;

int threadpool_init   (threadpool_t *tp, int queueDim,int nthreads_min, int nthreads_max);
int threadpool_submit (threadpool_t *tp, wi_function_t func, void *args);
int threadpool_destroy(threadpool_t *tp);

#endif
