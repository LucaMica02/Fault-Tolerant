#include "header.h"

void *barrier_thread(void *arg)
{
    /* A thread run the MPI_Barrier */
    BarrierArgs *args = (BarrierArgs *)arg;
    *(args->return_code) = MPI_Barrier(args->comm);

    /* Set the flag timed_out to false and wake up the main thread */
    pthread_mutex_lock(args->mutex);
    *(args->timed_out) = 0;
    pthread_cond_signal(args->cond);
    pthread_mutex_unlock(args->mutex);

    return NULL;
}

/* MPI_Barrier with a timeout associated */
int MPI_Barrier_timeout(MPI_Comm comm)
{
    pthread_t tid;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    int timed_out = 1;
    int return_code = 0;

    /* Create the thread */
    BarrierArgs args = {comm, &mutex, &cond, &timed_out, &return_code};
    pthread_create(&tid, NULL, barrier_thread, &args);

    /* Set the timeout */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += TIMEOUT;

    pthread_mutex_lock(&mutex);
    while (timed_out)
    {
        int rc = pthread_cond_timedwait(&cond, &mutex, &ts);
        if (rc == ETIMEDOUT)
        {
            /* If the timeout is reached abort the whole comm because a deadlock is detected */
            printf("DEADLOCK\n");
            pthread_mutex_unlock(&mutex);
            MPI_Abort(comm, 16); // MPI_ERR_OTHER
        }
    }

    pthread_mutex_unlock(&mutex);
    pthread_join(tid, NULL);
    return return_code;
}

void detect_failure(void *src, int send_size, MPI_Comm world_comm, MPI_Comm comm, Data *data, MPI_Datatype datatype, int *distance)
{
    int error = 0;
    error = MPI_Barrier_timeout(world_comm); // Detect failure at the previous step
    if (error != MPI_SUCCESS)
    {
        if (error != 75) // MPIX_ERR_PROC_FAILED
        {
            MPI_Abort(world_comm, error);
        }
        errhandler(&world_comm, &comm, distance, src, send_size, data, datatype);
    }
    else
    {
        if (data->dead_partner != -1)
        {
            printf("WRONG\n");
            MPI_Abort(world_comm, error);
        }
    }
    data->dead_partner = -1; // reset it
}

void reduce_pow2(void *src, void *dst, int send_size, MPI_Comm world_comm, Data *data, MPI_Datatype datatype, MPI_Op op)
{
    int p = (int)pow(2, floor(log2(data->original_size))); // closest lower power of two

    /* Mark the inactive ranks if there are */
    if (p < data->original_size)
    {
        data->active_ranks_count = p;
        data->inactive_ranks_count = data->original_size - p;
        data->active_ranks = (int *)realloc(data->active_ranks, data->active_ranks_count * sizeof(int));
        data->inactive_ranks = (int *)realloc(data->inactive_ranks, data->inactive_ranks_count * sizeof(int));
        for (int i = p; i < data->original_size; i++)
        {
            data->inactive_ranks[i - p] = i;
        }
        if (data->original_rank >= p)
        {
            data->active = 0;
        }
    }

    /* Inactive ranks send the data to their respective active rank */
    if (data->active == 0)
    {
        MPI_Send(src, send_size, datatype, data->active_ranks[data->original_rank - p], 0, world_comm);
    }
    else if (data->original_rank < data->inactive_ranks_count)
    {
        MPI_Recv(dst, send_size, datatype, data->inactive_ranks[data->original_rank], 0, world_comm, MPI_STATUS_IGNORE);
        MPI_Reduce_local(dst, src, send_size, datatype, op);
    }
}

int contains(int *array, int target, int n)
{
    /* Check if the given array of size n, contain the specified target */
    for (int i = 0; i < n; i++)
    {
        if (target == array[i])
        {
            return 1;
        }
    }
    return 0;
}

void check_abort(Data *data, int *ranks_gc, int nf, int distance, MPI_Comm pworld)
{
    /*
     * Check if all the ranks in one block are died or corrupted
     * in such case we don't have the data redundancy
     * so abort the whole communicator
     */
    int i, j, k;
    k = 0;
    for (i = 0; i < data->active_ranks_count; i++)
    {
        if (k == distance) // EDGE CASE
        {
            MPI_Abort(pworld, 16); // MPI_ERR_OTHER
        }
        if (i % distance == 0) // reset the block count
        {
            k = 0;
        }
        /* Check if the rank is died or corrupted */
        for (j = 0; j < nf; j++)
        {
            if (data->active_ranks[i] == ranks_gc[j] || data->active_ranks[i ^ (distance / 2)] == ranks_gc[j])
            {
                k++;
                break;
            }
        }
    }

    if (k == distance) // last check
    {
        MPI_Abort(pworld, 16); // MPI_ERR_OTHER
    }
}

int is_failed(int *ranks_gc, int *ranks, int n, int m)
{
    /*
     * Check if at least one item from ranks is also in ranks_gc
     * --> at least one rank is failed
     */
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < m; j++)
        {
            if (ranks_gc[i] == ranks[j])
                return 1;
        }
    }
    return 0;
}

/* ################## LOGS FOR DEBUG PURPOSE ################## */
void log_failed(int *ranks, int nf)
{
    printf("DEAD: ");
    for (int i = 0; i < nf; i++)
    {
        printf("%d ", ranks[i]);
    }
    puts("");
}

void log_struct(Data *data)
{
    printf("ACTIVE: ");
    for (int i = 0; i < data->active_ranks_count; i++)
    {
        printf("%d ", data->active_ranks[i]);
    }
    printf("\nINACTIVE: ");
    for (int i = 0; i < data->inactive_ranks_count; i++)
    {
        printf("%d ", data->inactive_ranks[i]);
    }
    puts("");
}