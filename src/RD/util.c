#include "header.h"

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