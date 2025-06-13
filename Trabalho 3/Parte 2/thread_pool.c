#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include "thread_pool.h"


// Inicializa a estrutura de buffer partilhado com a capacidade indicada.
void sharedBuffer_init (SharedBuffer *sb, int capacity)
{
    sb->buffer      = (void **)malloc(capacity * sizeof(void *));
    sb->dequeue     = 0;
    sb->enqueue     = 0;
    sb->nelems      = 0;
    sb->maxCapacity = capacity;

    pthread_cond_init(&sb->is_full_c,   NULL);
    pthread_cond_init(&sb->is_empty_c, NULL);

    pthread_mutex_init(&sb->mutex, NULL);
}

// Liberta todos os recursos associados ao buffer partilhado.
void sharedBuffer_destroy (SharedBuffer *sb)
{
    free(sb->buffer);

    pthread_cond_destroy(&sb->is_full_c);
    pthread_cond_destroy(&sb->is_empty_c);

    pthread_mutex_destroy(&sb->mutex);
}


// Insere um elemento no buffer (bloqueia se cheio).
void sharedBuffer_enqueue (SharedBuffer *sb, void *data)
{
    pthread_mutex_lock(&sb->mutex);

        while (sb->nelems == sb->maxCapacity) {  // buffer cheio
            pthread_cond_wait(&sb->is_full_c, &sb->mutex);
        }

        sb->buffer[sb->enqueue++] = data;

        if (sb->enqueue == sb->maxCapacity) 
            sb->enqueue = 0;

        ++sb->nelems;

        pthread_cond_signal(&sb->is_empty_c);

    pthread_mutex_unlock(&sb->mutex);
}


// Remove e devolve um elemento do buffer (bloqueia se vazio).
// @returns Ponteiro para o elemento removido.
void * sharedBuffer_dequeue (SharedBuffer *sb)
{
    void *ret;

    pthread_mutex_lock(&sb->mutex);

        while (sb->nelems == 0) {  // buffer vazio
            
            pthread_cond_wait(&sb->is_empty_c, &sb->mutex);
        }

        ret=sb->buffer[sb->dequeue++];

        if(sb->dequeue == sb->maxCapacity) 
            sb->dequeue = 0;

        --sb->nelems;

        pthread_cond_signal(&sb->is_full_c);

    pthread_mutex_unlock(&sb->mutex);

    return ret;
}


// Função executada por cada thread de trabalho
// @returns NULL
void *worker_job(void *arg)
{
    threadpool_t *tp = arg;

    while (1) {
        work_item_t *wi = sharedBuffer_dequeue(&tp->queue);
        if (wi == NULL)          // se for NULL é sinal para fechar o thread
            break;

        pthread_mutex_lock(&tp->lock);
        tp->nthreads_idle--;                 // está ativa 
        pthread_mutex_unlock(&tp->lock);

        wi->fn(wi->arg);                     // executar tarefa
        free(wi);

        pthread_mutex_lock(&tp->lock);
        tp->nthreads_idle++;                 // fica à espera
        pthread_mutex_unlock(&tp->lock);
    }

    pthread_mutex_lock(&tp->lock);
    tp->nthreads_alive--;
    pthread_mutex_unlock(&tp->lock);
    return NULL;
}

// inicia threadpool
// @returns 0 em sucesso e -1 em erro
int threadpool_init (threadpool_t *tp, int queueDim, int nthreads_min, int nthreads_max){

    if (!tp || queueDim <= 0 || nthreads_min <= 0 || nthreads_min > nthreads_max)
        return -1;
    
    tp->nthreads_min = nthreads_min;
    tp->nthreads_max = nthreads_max;
    tp->nthreads_alive = 0;
    tp->nthreads_idle = 0;
    tp->shutdown = 0;
    pthread_mutex_init(&tp->lock, NULL);

    tp->threads = malloc(nthreads_max * sizeof(pthread_t));
    if (!tp->threads) {
        pthread_mutex_destroy(&tp->lock);
        return -1;
    }

    sharedBuffer_init(&tp->queue, queueDim);

    // criar o numero min de threads
    for (int i = 0; i < nthreads_min; ++i) {
        if (pthread_create(&tp->threads[i], NULL, worker_job, tp) != 0) {
            threadpool_destroy(tp);
            return -1;
        }
        tp->nthreads_alive++;
        tp->nthreads_idle++;
    }
    return 0;

}

// Submete uma nova tarefa à threadpool
//
// cria mais threads se necessário.
// @returns 0 em sucesso e -1 em erro
int threadpool_submit (threadpool_t *tp, wi_function_t func, void *args){

    if (!tp || !func) return -1;

    work_item_t *wi = malloc(sizeof *wi);
    if (!wi) return -1;
    wi->fn  = func;
    wi->arg = args;

    sharedBuffer_enqueue(&tp->queue, wi);
   
    pthread_mutex_lock(&tp->lock);
    if (tp->nthreads_idle == 0 && tp->nthreads_alive < tp->nthreads_max) {
        if (pthread_create(&tp->threads[tp->nthreads_alive], NULL, worker_job, tp) == 0) {
            tp->nthreads_alive++;
            tp->nthreads_idle++;     
        }
    }
    pthread_mutex_unlock(&tp->lock);
    return 0;
}

// Termina a threadpool e espera que todas as threads terminem.
// @returns 0 em sucesso e -1 em erro
int threadpool_destroy (threadpool_t *tp){
    if (!tp) return -1;

    pthread_mutex_lock(&tp->lock);
    if (tp->shutdown) {                
        pthread_mutex_unlock(&tp->lock);
        return 0;
    }
    tp->shutdown = 1;
    int alive = tp->nthreads_alive;
    pthread_mutex_unlock(&tp->lock);

    // enviar NULL para acordar os threads que estão bloqueados no dequeue
    for (int i = 0; i < alive; ++i)
        sharedBuffer_enqueue(&tp->queue, NULL);

    for (int i = 0; i < alive; ++i)
        pthread_join(tp->threads[i], NULL);

    sharedBuffer_destroy(&tp->queue);
    free(tp->threads);
    pthread_mutex_destroy(&tp->lock);
    return 0;
}
