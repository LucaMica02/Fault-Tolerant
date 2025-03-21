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
} Data;

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

void errhandler(MPI_Comm *pcomm, int distance, int *src, int send_size, Data *data)
{
    MPI_Comm comm = *pcomm, comm_cpy, new_comm;
    int i, j, rank, size, nf, flag, p, total_count, block_count, to_wk_up, to_send, to_recv, corr;
    MPI_Group group_c, group_f, group_survivors;
    int *ranks_gc, *ranks_gf;

    to_wk_up = -1;
    to_send = -1;
    to_recv = -1;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* We use a combination of 'ack/get_acked' to obtain the list of
     * failed processes (as seen by the local rank).
     */
    MPIX_Comm_agree(comm, &flag);
    MPIX_Comm_failure_ack(comm);
    MPIX_Comm_failure_get_acked(comm, &group_f);
    MPI_Group_size(group_f, &nf);

    /* We use 'translate_ranks' to obtain the ranks of failed procs
     * in the input communicator 'comm'.
     */
    ranks_gf = (int *)malloc(nf * sizeof(int));
    ranks_gc = (int *)malloc(nf * sizeof(int));
    MPI_Comm_group(comm, &group_c);
    for (i = 0; i < nf; i++)
        ranks_gf[i] = i;
    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);

    // printf("From: %d / %d at It: %d and I have %d \n", rank, size, it, src);
    /*
    printf("From: %d / %d at It: %d Proc failed: %d\n", rank, size, it, nf);
    for (int i = 0; i < nf; i++)
    {
        printf("Rank %d failed\n", ranks_gc[i]);
    }
    */
    for (i = 0; i < nf; i++)
    {
        ranks_gc[i] = data->active_ranks[ranks_gc[i]];
    }

    // printf("from %d / %d;  %d %d\n", rank, size, nf, data->inactive_ranks_count);
    if (nf < data->inactive_ranks_count)
    {
        // printf("AAAAAAAAAAAAAAAAAAAA");
        //  CASO QUANDO INACTIVE RANK FALLISCE
        //  OCCHIO AL BEHAVIORAL DI FIXARE L'ARRAY
        //  call an inactive process
        //  1. Fix the array of survived ranks
        j = data->inactive_ranks_count - 1;
        for (i = 0; i < data->active_ranks_count; i++)
        {
            // printf("FAILED %d CURR %d\n", ranks_gc[0], data->active_ranks[i]);
            if (is_inactive(ranks_gc, data->active_ranks[i], nf) == 0)
            {
                // I have to wake up a new process
                if (((i % (distance * 2) == 0) && i + 1 == rank) ||
                    ((i % (distance * 2) != 0) && i - 1 == rank))
                {
                    to_wk_up = data->inactive_ranks[j];
                    printf("%d wake up %d\n", data->active_ranks[rank], data->inactive_ranks[j]);
                }
                // I have to send the data to the corrupted process
                corr = i ^ distance;
                if (((corr % (distance * 2) == 0) && corr + 1 == rank) ||
                    ((corr % (distance * 2) != 0) && corr - 1 == rank))
                {
                    to_send = data->active_ranks[corr];
                    printf("%d restore data of %d\n", data->active_ranks[rank], to_send);
                }
                // I'm a corrupted one
                if (corr == rank)
                    to_recv = 1;
                if (rank == 0)
                    printf("%d became %d\n", data->active_ranks[i], data->inactive_ranks[j]);
                data->active_ranks[i] = data->inactive_ranks[j];
                j--;
                // printf("%d wake up %d\n", data->active_ranks[rank], data->inactive_ranks[j]);
            }
            // printf("%d wake up %d\n", data->active_ranks[rank], data->inactive_ranks[j]);
        }
        data->inactive_ranks_count = j + 1;
        data->inactive_ranks = (int *)realloc(data->inactive_ranks, data->inactive_ranks_count * sizeof(int));
        if (rank == 0)
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

        // 2. create a new group with MPI_Group_incl
        MPI_Comm_group(MPI_COMM_WORLD, &group_c);
        MPI_Group_incl(group_c, data->active_ranks_count, data->active_ranks, &group_survivors);
        // printf("Err: %d from %d at GROUP\n", err, rank);

        MPIX_Comm_shrink(comm, &comm_cpy);
        // MPI_Comm_group(comm_cpy, &group_c);

        // 3. create the communicator with MPI_Comm_create
        MPI_Comm_create(comm_cpy, group_survivors, &new_comm);
        // printf("Err: %d from %d at COMM c: %d, s: %d \n", err, rank, i, p);

        // send the data to the wake up processes
        if (to_wk_up != -1)
        {
            printf("%d / %d blocked in wake up\n", rank, size);
            flag = 0;
            MPI_Send(&flag, 1, MPI_INT, to_wk_up, 0, MPI_COMM_WORLD);
            MPI_Send(src, send_size, MPI_INT, to_wk_up, 0, MPI_COMM_WORLD);
            MPI_Send(&data->active_ranks_count, 1, MPI_INT, to_wk_up, 0, MPI_COMM_WORLD);
            MPI_Send(&data->inactive_ranks_count, 1, MPI_INT, to_wk_up, 0, MPI_COMM_WORLD);
            MPI_Send(data->active_ranks, data->active_ranks_count, MPI_INT, to_wk_up, 0, MPI_COMM_WORLD);
            MPI_Send(data->inactive_ranks, data->inactive_ranks_count, MPI_INT, to_wk_up, 0, MPI_COMM_WORLD);
            MPI_Send(&distance, 1, MPI_INT, to_wk_up, 0, MPI_COMM_WORLD);
        }
        if (to_send != -1)
        {
            printf("%d / %d blocked in send up\n", rank, size);
            MPI_Send(src, send_size, MPI_INT, to_wk_up, 0, MPI_COMM_WORLD);
        }
        if (to_recv == 1)
        {
            printf("%d / %d blocked in receiving up\n", rank, size);
            MPI_Recv(src, send_size, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    else
    {
        // shrink the communicator
        // 1. Array of survived ranks
        p = (int)pow(2, floor(log2(size - nf)));
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
            // printf("%d %d %d %d\n", distance, total_count, p, i);
            if (i % (distance * 2) == 0)
            {
                block_count = 0;
            }
            if (block_count < distance && is_active(ranks_gc, i, distance, nf) == 1)
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

        // 2. create a new group with MPI_Group_incl
        MPI_Group_incl(group_c, p, data->active_ranks, &group_survivors);
        // printf("Err: %d from %d at GROUP\n", err, rank);

        MPIX_Comm_shrink(comm, &comm_cpy);
        // MPI_Comm_group(comm_cpy, &group_c);

        // 3. create the communicator with MPI_Comm_create
        MPI_Comm_create(comm_cpy, group_survivors, &new_comm);
        // printf("Err: %d from %d at COMM c: %d, s: %d \n", err, rank, i, p);
    }

    free(ranks_gf);
    free(ranks_gc);
    *pcomm = new_comm;

    data->active = 0;
    for (i = 0; i < p; i++)
    {
        if (data->active_ranks[i] == rank)
        {
            data->active = 1;
            break;
        }
    }
}

void recursive_doubling_int_sum(int *src, int *dst, int send_size, MPI_Comm comm, Data *data)
{
    int rank, size, distance, error, partner, flag, cc;
    int *src_copy, *dst_copy;
    MPI_Group world_group, new_group;
    MPI_Comm comm_cpy;

    src_copy = (int *)malloc(sizeof(int) * send_size);
    dst_copy = (int *)malloc(sizeof(int) * send_size);

    memcpy(src_copy, src, sizeof(int) * send_size);
    memcpy(dst_copy, dst, sizeof(int) * send_size);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    error = 0;
    distance = 1;
    cc = 0;
    flag = -1;

// Perform recursive doubling
jump_here:
    for (; distance < size; distance *= 2)
    {
        error = MPI_Barrier(comm); // fault detection
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if ((distance == 2 && rank == 2))
            raise(SIGKILL);
        MPI_Comm_rank(comm, &rank);
        if (error != MPI_SUCCESS)
        {
            distance /= 2;
            cc = 1;
            errhandler(&comm, distance, src_copy, send_size, data);
            if (data->active == 0)
            {
                break;
            }
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &size);
            // printf("From: %d / %d at It: %d and I have %d \n", rank, size, distance, src_copy[0]);
        }
        partner = rank ^ distance;
        if (partner < size)
        {
            MPI_Sendrecv(src_copy, send_size, MPI_INT, partner, 0,
                         dst_copy, send_size, MPI_INT, partner, 0,
                         comm, MPI_STATUS_IGNORE);
            for (int i = 0; i < send_size; i++)
            {
                src_copy[i] += dst_copy[i];
            }
        }
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if ((distance == 2 && rank == 8 && cc == 1))
            raise(SIGKILL);
        // printf("at step %d code %d\n", distance, error);
    }

    if (data->active == 0)
    {
        MPI_Status status;
        // printf("%d / %d flag is %d\n", rank, size, flag);
        //  waiting for flag
        error = MPI_Recv(&flag, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        printf("%d / %d waiting for flag and received %d and %d from %d with err: %d\n", rank, size, flag, src_copy[0], status.MPI_SOURCE, error);
        // waiting for result
        MPI_Recv(src_copy, send_size, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (flag == 0) // came back to computation
        {
            // reconstruct data
            MPI_Recv(&data->active_ranks_count, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&data->inactive_ranks_count, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            data->active_ranks = (int *)realloc(data->active_ranks, data->active_ranks_count * sizeof(int));
            data->inactive_ranks = (int *)realloc(data->inactive_ranks, data->inactive_ranks_count * sizeof(int));
            MPI_Recv(data->active_ranks, data->active_ranks_count, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(data->inactive_ranks, data->inactive_ranks_count, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // reconstruct the communicator
            MPIX_Comm_shrink(MPI_COMM_WORLD, &comm_cpy);
            MPI_Comm_group(comm_cpy, &world_group);
            MPI_Group_incl(world_group, data->active_ranks_count, data->active_ranks, &new_group);
            MPI_Comm_create(MPI_COMM_WORLD, new_group, &comm);
            // recv distance
            MPI_Recv(&distance, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // set error
            error = 0;
            // calc size and rank
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &size);
            // jump to for loop
            goto jump_here;
        }
    }
    else
    {
        MPI_Comm_rank(comm, &rank);
        if (rank < data->inactive_ranks_count)
        {
            flag = 1;
            MPI_Send(&flag, 1, MPI_INT, data->inactive_ranks[rank], 0, MPI_COMM_WORLD);
            MPI_Send(src_copy, send_size, MPI_INT, data->inactive_ranks[rank], 0, MPI_COMM_WORLD);
        }
    }

    *dst = *src_copy;
    free(src_copy);
    free(dst_copy);
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int size, rank, res;
    Data *data = (Data *)malloc(sizeof(Data));

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

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
    recursive_doubling_int_sum(&rank, &res, 1, MPI_COMM_WORLD, data);
    printf("Hello from %d of %d and the result is: %d \n", rank, size, res);
    MPI_Finalize();
    free(data->inactive_ranks);
    free(data->active_ranks);
    free(data);
    return 0;
}