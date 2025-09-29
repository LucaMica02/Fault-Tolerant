#include "header.h"

/*
 * Recursive Doubling Allreduce standard implementation with Fault Tolerance
 */
void recursive_doubling(void *src, void *dst, int send_size, MPI_Comm world_comm, MPI_Comm comm, Data *data, MPI_Datatype datatype, MPI_Op op)
{
    int rank, size, distance, partner, type_size, error;

    MPI_Type_size(datatype, &type_size);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    reduce_pow2(src, dst, send_size, world_comm, data, datatype, op); // Reduce to power of two

    MPI_Comm_set_errhandler(world_comm, // Tolerate failure
                            MPI_ERRORS_RETURN);
    MPI_Barrier(world_comm);

    /* Recursive Doubling Body */
    for (distance = 1; distance < data->active_ranks_count; distance *= 2)
    {
        /* Compute the right partner */
        if (data->active == 1)
            MPI_Comm_rank(comm, &rank);
        partner = rank ^ distance;

        /* Exchange the data between ranks */
        if (partner < size && data->active != 0)
        {
            MPI_Request req;
            MPI_Isend(src, send_size, datatype, partner, 0, comm, &req);
            MPI_Recv(dst, send_size, datatype, partner, 0, comm, MPI_STATUS_IGNORE);
            MPI_Wait(&req, MPI_STATUS_IGNORE);

            /*
             * We accumulate the result in src to send it at the parner in the next iteration
             * to avoid doing the memcpy at the end, if is the last reduction take the dst like a out buffer
             */
            if (distance * 2 >= data->active_ranks_count)
            {
                MPI_Reduce_local(src, dst, send_size, datatype, op);
            }
            else
            {
                MPI_Reduce_local(dst, src, send_size, datatype, op);
            }
        }
        int flag = 1;
        MPIX_Comm_agree(world_comm, &flag); // synchronization
        error = MPI_Barrier(world_comm); // Detect failure at the previous step
        if (error != MPI_SUCCESS)
        {
            if (error != 75) // MPIX_ERR_PROC_FAILED
            {
                MPI_Abort(world_comm, error);
            }
            distance *= 2;
            if (distance >= data->active_ranks_count) { // At the last step we switch src with dst
                errhandler(&world_comm, &comm, &distance, dst, send_size, data, datatype);
            } else {
                errhandler(&world_comm, &comm, &distance, src, send_size, data, datatype);
            }
            distance /= 2;
        }
    }

    MPI_Comm_set_errhandler(world_comm, // no more tolerating failure
                            MPI_ERRORS_ARE_FATAL);
    MPI_Barrier(world_comm);

    /* Active ranks send back result to inactive ones */
    if (data->active == 0)
    {
        // waiting for result
        MPI_Recv(dst, send_size, datatype, MPI_ANY_SOURCE, 0, world_comm, MPI_STATUS_IGNORE);
    }
    else
    {
        MPI_Comm_rank(comm, &rank);
        if (rank < data->inactive_ranks_count)
        {
            // send the result
            MPI_Send(dst, send_size, datatype, data->inactive_ranks[rank], 0, world_comm);
        }
    }
}

/*
 * Main body to test the recursive doubling
 */
int main(int argc, char *argv[])
{
    clock_t start, end;
    double difftime;
    int size, rank, res;
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int buf_size = atoi(argv[1]);
    int *buffer;
    int *result;
    buffer = (int *)malloc(buf_size * sizeof(int));
    result = (int *)malloc(buf_size * sizeof(int));
    for (int i = 0; i < buf_size; i++)
    {
        buffer[i] = rank;
    }

    start = clock();
    Data *data = (Data *)malloc(sizeof(Data));
    data->original_rank = rank;
    data->original_size = size;
    data->active = 1;
    data->active_ranks_count = size;
    data->inactive_ranks_count = 0;
    data->active_ranks = (int *)malloc(sizeof(int) * data->active_ranks_count);
    data->inactive_ranks = (int *)malloc(sizeof(int));
    data->dead_partner = -1;
    for (int i = 0; i < size; i++)
    {
        data->active_ranks[i] = i;
    }

    recursive_doubling(buffer, result, buf_size, MPI_COMM_WORLD, MPI_COMM_WORLD, data, MPI_INT, MPI_SUM);
    MPI_Barrier(MPI_COMM_WORLD);
    end = clock();
    difftime = ((double)(end - start)) / CLOCKS_PER_SEC;

    res = 0;
    for (int i = 0; i < buf_size; i++)
    {
        res += (result[i] % 17);
    }

    if (rank == 0) {
        printf("P: %d\n", size);
        printf("Size: %d\n", buf_size);
        printf("Time: %lf\n", difftime);
    }
    printf("Hello from %d of %d and the result is: %d\n", data->original_rank, data->original_size, res);

    if (data->inactive_ranks != NULL)
        free(data->inactive_ranks);
    if (data->active_ranks != NULL)
        free(data->active_ranks);
    if (data != NULL)
        free(data);
    if (buffer != NULL)
        free(buffer);
    if (result != NULL)
        free(result);
    MPI_Finalize();
    return 0;
}