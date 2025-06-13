#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <limits.h>



typedef struct {

    pthread_cond_t wait_cond; 
    int nthreads, count, phase;
    pthread_mutex_t mutex;


} sot_barrier_t;

// inicializar sot_barrier
// @returns 0 em sucesso e -1 em erro
int sot_barrier_init (sot_barrier_t *barrier, int numberOfThreads){

    barrier->nthreads = numberOfThreads;
    barrier->count = barrier->phase = 0;

    if(pthread_cond_init(&barrier->wait_cond,NULL) != 0){
        return -1;
    }
    pthread_mutex_init(&barrier->mutex,NULL);
    
    return 0;
}

// destruir sot_barrier
// @returns 0 em sucesso e -1 em erro
int sot_barrier_destroy (sot_barrier_t *barrier){

    if(pthread_cond_destroy(&barrier->wait_cond) != 0)
        return -1;
    else if(pthread_mutex_destroy(&barrier->mutex) != 0)
        return -1;
    return 0;
}

// espera até todas a threads chegarem à barreira
void sot_barrier_wait (sot_barrier_t *barrier){

    pthread_mutex_lock(&barrier->mutex);

    int my_phase = barrier->phase;

    if (++barrier->count == barrier->nthreads)
    {   
        barrier->count = 0;
        barrier->phase++;
        pthread_cond_broadcast(&barrier->wait_cond);

    } else
    {// espera até ao if mudar de fase, isto previne uma race condition que acontece se fizermos while(b->count != 0)
        while (my_phase == barrier->phase) { 
            pthread_cond_wait(&barrier->wait_cond, &barrier->mutex);
        }
    }
    
    pthread_mutex_unlock(&barrier->mutex);
}

typedef struct
{
    int* array;
    size_t len;
    int *g_min;
    int *g_max;
    double *average;
    int v_size;  // tamanho do array original para a média
    pthread_mutex_t* mutex;
    sot_barrier_t* barrier;

} vec_t;

void * thread_work(void* arg){
    vec_t* vec = (vec_t*)arg;
    int min = INT_MAX, max = INT_MIN;
    double media = 0;  

    // ver minimo e maximo local
    for (size_t i = 0; i < vec->len; i++)
    {
        if (vec->array[i] < min)
        {
            min = vec->array[i];
        }
        if (vec->array[i] > max)
        {
            max = vec->array[i];
        }
            
    }
    pthread_mutex_lock(vec->mutex);

        if (*vec->g_min > min)
        {
            *vec->g_min = min;
        }
        if (*vec->g_max < max)
        {
            *vec->g_max = max;
        }
    
    pthread_mutex_unlock(vec->mutex);

    sot_barrier_wait(vec->barrier);
    //  2ª fase
    //  normalização e media
    for (int i = 0; i < vec->len; i++)
    {
        // certificar que não se divide por 0
        if (*vec->g_max - *vec->g_min == 0) {
            vec->array[i] = 0; // se todos os elemntos são iguais, normalizar a 0
        } else {
            vec->array[i] = ((vec->array[i] - *vec->g_min) * 100) / (*vec->g_max - *vec->g_min);
        }
        media += vec->array[i];
    }
    media /= vec->v_size;
    pthread_mutex_lock(vec->mutex);

        *vec->average += media;

    pthread_mutex_unlock(vec->mutex);
    sot_barrier_wait(vec->barrier);
    //  3ª fase
    //  classificação
    for (int i = 0; i < vec->len; i++)
    {
        if (vec->array[i] >= *vec->average)
        {
            vec->array[i] = 1;
        } else{
            vec->array[i] = 0;
        }
        
    }
    
    return NULL;
}

void norm_min_max_and_classify_parallel(int v[], size_t v_sz, int nThreads)
{    
    pthread_t threads[nThreads];
    vec_t args[nThreads];
    int g_max = INT_MIN; int g_min = INT_MAX;
    double g_average = 0;
    pthread_mutex_t m_mutex;
    sot_barrier_t m_barrier;

    vec_t m_vec = {NULL,v_sz,&g_min,&g_max,&g_average,v_sz,&m_mutex,&m_barrier};

    int start = 0, len;
    int base = v_sz / nThreads;
    int remainder = v_sz % nThreads;

    pthread_mutex_init(&m_mutex, NULL);
    sot_barrier_init(&m_barrier,nThreads);
    
    // dividir o array para os threads
    for (int i = 0; i < nThreads; i++)
    {
        len = base - 1 + (i < remainder); // adicionar resto
        args[i] = m_vec;
        args[i].array = v + start;
        args[i].len = len + 1;
        pthread_create(&threads[i],NULL,thread_work,(void *)&args[i]);
        start += len + 1;
    }
    for (int i = 0; i < nThreads; i++)
    {
        pthread_join(threads[i],NULL);
    }
    pthread_mutex_destroy(&m_mutex);
    sot_barrier_destroy(&m_barrier);
} 

int main(int argc, char const *argv[])
{
    int arr[] = { 0, 25, 50, 75, 100 };
    int arr_sz = sizeof(arr) / sizeof(arr[0]);
    norm_min_max_and_classify_parallel(arr,arr_sz,3);
    for (int i = 0; i < arr_sz; i++)
    {
        printf("%d ",arr[i]);
    }
    
    return 0;
}
