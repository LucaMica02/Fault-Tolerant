#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <mpi.h>
#include <mpi-ext.h>

typedef struct
{
    int active_ranks_count;
    int *active_ranks;
    int inactive_ranks_count;
    int *inactive_ranks;
    int active;
    int original_rank;
    int original_size;
} Data;

void check_abort(Data *data, int *ranks_gc, int nf, int distance, MPI_Comm pworld)
{
    int i, j, k;
    k = 0;
    for (i = 0; i < data->active_ranks_count; i++)
    {
        if (k == distance)
        {
            MPI_Abort(pworld, 16); // MPI_ERR_OTHER
        }
        if (i % distance == 0)
        {
            k = 0;
        }
        for (j = 0; j < nf; j++)
        {
            if (data->active_ranks[i] == ranks_gc[j] || data->active_ranks[i ^ (distance / 2)] == ranks_gc[j])
            {
                k++;
                break;
            }
        }
    }
    if (k == distance)
    {
        MPI_Abort(pworld, 16); // MPI_ERR_OTHER
    }
}

int is_failed(int *ranks_gc, int *ranks, int n, int m)
{
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

int is_inactive(int *ranks_gc, int rank, int n)
{
    // rank not in ranks_gc
    for (int i = 0; i < n; i++)
    {
        if ((rank == ranks_gc[i]))
        {
            return 0;
        }
    }
    return 1;
}

int is_active(int *ranks_gc, int rank, int distance, int n)
{
    // rank not in ranks_gc
    // rank ^ distance not in ranks_gc
    for (int i = 0; i < n; i++)
    {
        if ((rank == ranks_gc[i]) || ((rank ^ distance) == ranks_gc[i]))
        {
            return 0;
        }
    }
    return 1;
}

void errhandler(MPI_Comm *pworld, MPI_Comm *pcomm, int *distance, int *src, int send_size, Data *data, MPI_Datatype msg_type)
{

    MPI_Comm new_world, new_comm, old_comm;
    int i, j, k, rank, size, nf, p, total_count, block_count, to_recv, wk_up, corr, err, inactive_nf;
    MPI_Group group_c, group_f, group_survivors;
    int *ranks_gc, *ranks_gf;

    int to_wk_up[2] = {-1, -1};
    int to_send[2] = {-1, -1};
    to_recv = -1;
    wk_up = 0;

    MPIX_Comm_agree(*pworld, &err); // for syncronize error detection with the ack
    MPI_Comm_set_errhandler(*pworld,
                            MPI_ERRORS_ARE_FATAL);

    /* Check who is failed in the world comm and restore it */
    MPI_Comm_rank(*pworld, &rank);
    MPI_Comm_size(*pworld, &size);

    // MPIX_Comm_agree(*pworld, &err);
    MPIX_Comm_failure_ack(*pworld);
    MPIX_Comm_failure_get_acked(*pworld, &group_f);
    MPI_Group_size(group_f, &nf);

    ranks_gf = (int *)malloc(nf * sizeof(int));
    ranks_gc = (int *)malloc(nf * sizeof(int));
    MPI_Comm_group(*pworld, &group_c);
    for (i = 0; i < nf; i++)
        ranks_gf[i] = i;
    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);

    MPIX_Comm_shrink(*pworld, &new_world);
    old_comm = *pworld;
    *pworld = new_world;

    if (data->original_rank == 0)
    {
        for (i = 0; i < nf; i++)
        {
            printf("%d ", ranks_gc[i]);
        }
        puts("");
    }

    inactive_nf = 0;
    // check if is failed an inactive rank
    if (data->inactive_ranks_count > 0 && is_failed(ranks_gc, data->inactive_ranks, nf, data->inactive_ranks_count) == 1)
    {
        // fix inactive ranks data
        for (i = 0; i < data->inactive_ranks_count; i++)
        {
            if (is_inactive(ranks_gc, data->inactive_ranks[i], nf) == 0)
            {
                inactive_nf++;
            }
            else
            {
                data->inactive_ranks[i - inactive_nf] = data->inactive_ranks[i];
            }
        }
        data->inactive_ranks_count -= inactive_nf;
        data->inactive_ranks = (int *)realloc(data->inactive_ranks, data->inactive_ranks_count * sizeof(int));
    }

    // check if is failed an active rank
    if (is_failed(ranks_gc, data->active_ranks, nf, data->active_ranks_count) == 1)
    {
        // check if all the process in one block are failed
        if ((nf - inactive_nf) >= (*distance / 2)) // previous check
        {
            check_abort(data, ranks_gc, nf, *distance, *pworld);
        }

        nf -= inactive_nf;
        // check if we have enough inactive nodes
        if (nf <= data->inactive_ranks_count)
        {
            // check if you are inactive node that will be waked up
            int pos = -1;
            for (i = 0; i < data->inactive_ranks_count; i++)
            {
                if (data->inactive_ranks[i] == rank)
                {
                    pos = i;
                    break;
                }
            }
            if (pos >= data->inactive_ranks_count - nf)
                wk_up = 1;

            j = data->inactive_ranks_count - 1;
            for (i = 0; i < data->active_ranks_count; i++)
            {
                if (is_inactive(ranks_gc, data->active_ranks[i], nf + inactive_nf) == 0)
                {
                    if (data->active == 1)
                    {
                        MPI_Comm_rank(*pcomm, &rank);
                        // I have to wake up a new process
                        if (((i % (*distance) == 0) && i + 1 == rank) ||
                            ((i % (*distance) != 0) && i - 1 == rank))
                        {
                            (to_wk_up[0] == -1) ? (to_wk_up[0] = data->inactive_ranks[j]) : (to_wk_up[1] = data->inactive_ranks[j]);
                            // printf("%d wake up %d\n", data->active_ranks[rank], data->inactive_ranks[j]);
                        }
                        // I have to send the data to the corrupted process
                        corr = i ^ (*distance / 2);
                        if (((corr % (*distance) == 0) && corr + 1 == rank) ||
                            ((corr % (*distance) != 0) && corr - 1 == rank))
                        {
                            (to_send[0] == -1) ? (to_send[0] = corr) : (to_send[1] = corr);
                            // printf("%d restore data of %d\n", data->active_ranks[rank], corr);
                        }
                        // I'm a corrupted one
                        if (rank == (i ^ (*distance / 2)))
                        {
                            if (rank % (*distance) == 0)
                            {
                                to_recv = rank + 1;
                            }
                            else
                            {
                                to_recv = rank - 1;
                            }
                            // printf("rank to recv %d\n", to_recv);
                        }
                        // if (rank == 0)
                        //  printf("%d became %d\n", data->active_ranks[i], data->inactive_ranks[j]);
                    }
                    data->active_ranks[i] = data->inactive_ranks[j];
                    j--;
                }
            }
            data->inactive_ranks_count = j + 1;
            data->inactive_ranks = (int *)realloc(data->inactive_ranks, data->inactive_ranks_count * sizeof(int));
        }
        else
        {
            // shrink the communicator
            // 1. Array of survived ranks
            p = (int)pow(2, floor(log2(data->active_ranks_count - nf)));
            k = (data->active_ranks_count / p);
            *distance /= k;
            data->inactive_ranks_count = data->active_ranks_count - p - nf;
            data->inactive_ranks = (int *)realloc(data->inactive_ranks, data->inactive_ranks_count * sizeof(int));
            data->active_ranks_count = p;
            data->active_ranks = (int *)realloc(data->active_ranks, data->active_ranks_count * sizeof(int));
            total_count = 0;
            block_count = 0;
            i = 0;
            j = 0;
            while (i < size)
            {
                if (i % (*distance * k) == 0)
                {
                    block_count = 0;
                }
                if (block_count < *distance && is_active(ranks_gc, i, *distance, nf) == 1)
                {
                    data->active_ranks[total_count] = i;
                    total_count++;
                    block_count++;
                }
                else if (is_inactive(ranks_gc, i, nf) == 1)
                {
                    data->inactive_ranks[j] = i;
                    j++;
                }
                i += 1;
            }
        }

        // log the data struct
        if (data->original_rank == 1)
        {
            for (int i = 0; i < data->active_ranks_count; i++)
            {
                printf("%d ", data->active_ranks[i]);
            }
            puts("");
            for (int i = 0; i < data->inactive_ranks_count; i++)
            {
                printf("%d ", data->inactive_ranks[i]);
            }
            puts("");
        }

        MPI_Comm_group(old_comm, &group_c);
        MPI_Group_incl(group_c, data->active_ranks_count, data->active_ranks, &group_survivors);
        MPI_Comm_create(*pworld, group_survivors, &new_comm);
        // printf("%d / %d got: %d\n", data->original_rank, data->original_size, err);
        *pcomm = new_comm;

        // restore the data if necessary
        if (to_wk_up[0] != -1)
        {
            err = MPI_Send(src, send_size, msg_type, to_wk_up[0], 0, old_comm);
            printf("%d / %d blocked in sendig to inactive %d err: %d\n", data->original_rank, data->original_size, to_wk_up[0], err);
        }
        if (to_wk_up[1] != -1)
        {
            err = MPI_Send(src, send_size, msg_type, to_wk_up[1], 0, old_comm);
            printf("%d / %d blocked in sendig to inactive %d err: %d\n", data->original_rank, data->original_size, to_wk_up[1], err);
        }
        if (to_send[0] != -1)
        {
            err = MPI_Send(src, send_size, msg_type, to_send[0], 0, *pcomm);
            printf("%d / %d blocked in send up %d err: %d \n", data->original_rank, data->original_size, to_send[0], err);
        }
        if (to_send[1] != -1)
        {
            err = MPI_Send(src, send_size, msg_type, to_send[1], 0, *pcomm);
            printf("%d / %d blocked in send up %d err: %d \n", data->original_rank, data->original_size, to_send[1], err);
        }
        if (to_recv != -1)
        {
            err = MPI_Recv(src, send_size, msg_type, to_recv, 0, *pcomm, MPI_STATUS_IGNORE);
            printf("%d / %d blocked in receiving up %d err: %d recv: %d\n", data->original_rank, data->original_size, to_recv, err, src[0]);
        }
        if (wk_up == 1)
        {
            printf("%d / %d inactive blocked in receiving up err: %d recv: %d\n", data->original_rank, data->original_size, err, src[0]);
            err = MPI_Recv(src, send_size, msg_type, MPI_ANY_SOURCE, 0, old_comm, MPI_STATUS_IGNORE);
        }
    }

    // restore data struct
    for (i = 0; i < data->active_ranks_count; i++)
    {
        k = 0;
        for (j = 0; j < nf + inactive_nf; j++)
        {
            if (data->active_ranks[i] > ranks_gc[j])
            {
                k++;
            }
        }
        data->active_ranks[i] -= k;
    }
    for (i = 0; i < data->inactive_ranks_count; i++)
    {
        k = 0;
        for (j = 0; j < nf + inactive_nf; j++)
        {
            if (data->inactive_ranks[i] > ranks_gc[j])
            {
                k++;
            }
        }
        data->inactive_ranks[i] -= k;
    }

    // check if we are still active or not
    MPI_Comm_rank(*pworld, &rank);
    data->active = 0;
    for (i = 0; i < data->active_ranks_count; i++)
    {
        if (data->active_ranks[i] == rank)
        {
            data->active = 1;
            break;
        }
    }
    // if ((data->original_rank == 13))
    //   raise(SIGKILL);
    // MPI_Group_free(&group_survivors); // error if an inactive proc fail
    MPI_Group_free(&group_c);
    MPI_Group_free(&group_f);
    free(ranks_gf);
    free(ranks_gc);
    MPI_Barrier(*pworld);
    MPI_Comm_set_errhandler(*pworld,
                            MPI_ERRORS_RETURN);
}

void recursive_doubling_int_sum(int *src, int *dst, int send_size, MPI_Comm world_comm, MPI_Comm comm, Data *data, MPI_Datatype msg_type)
{
    int rank, size, distance, error, partner, cc;
    int *src_copy, *dst_copy;

    src_copy = (int *)malloc(sizeof(int) * send_size);
    dst_copy = (int *)malloc(sizeof(int) * send_size);

    memcpy(src_copy, src, sizeof(int) * send_size);
    memcpy(dst_copy, dst, sizeof(int) * send_size);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    error = 0;
    cc = 0;

    // Perform recursive doubling
    for (distance = 1; distance < data->active_ranks_count; distance *= 2)
    {
        // printf("EEE dist: %d size: %d\n", distance, data->active_ranks_count);
        //  printf("%d / %d & D: %d\n", rank, size, distance);
        if ((distance == 2 && data->original_rank == 2))
            raise(SIGKILL);
        if (error != MPI_SUCCESS)
        {
            if (error != 75)
            {
                MPI_Abort(world_comm, error);
            }
            cc = 1;
            errhandler(&world_comm, &comm, &distance, src_copy, send_size, data, msg_type);
            // printf("From: %d / %d at It: %d and I have %d \n", rank, size, distance, src_copy[0]);
        }
        // printf("%d / %d dist: %d cc: %d \n", data->original_rank, data->original_size, distance, cc);
        if (data->active == 1)
        {
            MPI_Comm_rank(comm, &rank);
            // printf("%d / %d is here! %d %d\n", rank, size, a, b);
        }
        partner = rank ^ distance;
        if (partner < size && data->active != 0)
        {
            MPI_Sendrecv(src_copy, send_size, msg_type, partner, 0,
                         dst_copy, send_size, msg_type, partner, 0,
                         comm, MPI_STATUS_IGNORE);
            for (int i = 0; i < send_size; i++)
            {
                src_copy[i] += dst_copy[i];
            }
            // printf("%d / %d res: %d step: %d\n", data->original_rank, data->original_size, src_copy[0], distance);
        }
        // printf("at step %d code %d\n", distance, error);
        // printf("EEE\n");
        // printf("%d / %d waiting for result and I'm %d\n", data->original_rank, data->original_size, data->active);
        if ((distance == 4 && data->original_rank == 1 && cc == 1) || (distance == 4 && data->original_rank == 4 && cc == 1) || (distance == 4 && data->original_rank == 8 && cc == 1) || (distance == 4 && data->original_rank == 12 && cc == 1))
            raise(SIGKILL);
        error = MPI_Barrier(world_comm); // fault detection
    }

    if (error != MPI_SUCCESS)
    {
        if (error != 75)
        {
            MPI_Abort(world_comm, error);
        }
        cc = 1;
        errhandler(&world_comm, &comm, &distance, src_copy, send_size, data, msg_type);
        printf("AAAAA\n");
    }
    // printf("AAAAA\n");
    if (data->active == 0)
    {
        // waiting for result
        MPI_Status status;
        // printf("AAAAA\n");
        MPI_Recv(src_copy, send_size, msg_type, MPI_ANY_SOURCE, 0, world_comm, &status);
        // printf("%d / %d received %d from %d\n", rank, size, src_copy[0], status.MPI_SOURCE);
    }
    else
    {
        MPI_Comm_rank(comm, &rank);
        if (rank < data->inactive_ranks_count)
        {
            // printf("AAAAA\n");
            MPI_Send(src_copy, send_size, msg_type, data->inactive_ranks[rank], 0, world_comm);
            // printf("%d / %d will send %d to %d\n", rank, size, src_copy[0], data->inactive_ranks[rank]);
        }
    }

    *dst = *src_copy;
    free(src_copy);
    free(dst_copy);
}

void recursive_doubling(int *src, int *dst, int send_size, MPI_Comm world_comm, MPI_Comm comm, Data *data, MPI_Datatype msg_type)
{
    // calculate p'
    int p = (int)pow(2, floor(log2(data->original_size)));
    // mark the inactive rank
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
    // send data to the active rank
    if (data->active == 0)
    {
        MPI_Send(src, send_size, msg_type, data->active_ranks[data->original_rank - p], 0, world_comm);
        // printf("From %d / %d and I send: %d\n", data->original_rank, data->original_size, src[0]);
    }
    else if (data->original_rank < data->inactive_ranks_count)
    {
        MPI_Recv(dst, send_size, msg_type, data->inactive_ranks[data->original_rank], 0, world_comm, MPI_STATUS_IGNORE);
        // printf("From %d / %d and I received: %d\n", data->original_rank, data->original_size, dst[0]);
        for (int i = 0; i < send_size; i++)
        {
            src[i] += dst[i];
        }
    }
    // printf("From %d / %d I'm: %d and I have: %d\n", data->original_rank, data->original_size, data->active, src[0]);
    //   performe the algorithm
    recursive_doubling_int_sum(src, dst, send_size, world_comm, comm, data, msg_type);
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int size, rank, res;
    Data *data = (Data *)malloc(sizeof(Data));

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    data->original_rank = rank;
    data->original_size = size;
    data->active = 1;
    data->active_ranks_count = size;
    data->inactive_ranks_count = 0;
    data->active_ranks = (int *)malloc(sizeof(int) * data->active_ranks_count);
    data->inactive_ranks = (int *)malloc(sizeof(int));
    for (int i = 0; i < size; i++)
    {
        data->active_ranks[i] = i;
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            MPI_ERRORS_RETURN);
    MPI_Barrier(MPI_COMM_WORLD);

    // MPI_Allreduce(&rank, &res, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    recursive_doubling(&rank, &res, 1, MPI_COMM_WORLD, MPI_COMM_WORLD, data, MPI_INT);
    printf("Hello from %d of %d and the result is: %d \n", data->original_rank, data->original_size, res);
    if (data->original_rank == 0)
    {
        printf("Expected: %d\n", ((data->original_size - 1) * data->original_size) / 2);
    }
    MPI_Finalize();
    free(data->inactive_ranks);
    free(data->active_ranks);
    free(data);
    return 0;
}