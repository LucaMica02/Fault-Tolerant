#include "header.h"

/*
 * Recursive Doubling Allreduce standard implementation with Fault Tolerance
 */
void recursive_doubling_v1(void *src, void *dst, int send_size, MPI_Comm world_comm, MPI_Comm comm, Data *data, MPI_Datatype datatype, MPI_Op op)
{
    int rank, size, distance, error, partner, type_size;
    void *src_copy, *dst_copy;

    MPI_Type_size(datatype, &type_size);
    src_copy = (void *)malloc(type_size * send_size);
    dst_copy = (void *)malloc(type_size * send_size);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    error = 0;

    reduce_pow2(src, dst, send_size, world_comm, data, datatype, op); // Reduce to power of two
    memcpy(src_copy, src, type_size * send_size);
    memcpy(dst_copy, dst, type_size * send_size);

    MPI_Comm_set_errhandler(world_comm, // Tolerate failure
                            MPI_ERRORS_RETURN);
    MPI_Barrier(world_comm);

    /* Recursive Doubling Body */
    for (distance = 1; distance < data->active_ranks_count; distance *= 2)
    {
        error = MPI_Barrier(world_comm); // Detect failure at the previous step
        if (error != MPI_SUCCESS)
        {
            if (error != 75) // MPIX_ERR_PROC_FAILED
                MPI_Abort(world_comm, error);
            errhandler(&world_comm, &comm, &distance, src_copy, send_size, data, datatype);
        }

        /* Compute the right partner */
        if (data->active == 1)
            MPI_Comm_rank(comm, &rank);
        partner = rank ^ distance;

        /* Exchange the data between ranks */
        if (partner < size && data->active != 0)
        {
            MPI_Sendrecv(src_copy, send_size, datatype, partner, 0,
                         dst_copy, send_size, datatype, partner, 0,
                         comm, MPI_STATUS_IGNORE);
            MPI_Reduce_local(dst_copy, src_copy, send_size, datatype, op);
        }
    }

    /* Detect failure at the last step */
    error = MPI_Barrier(world_comm);
    if (error != MPI_SUCCESS)
    {
        if (error != 75) // MPIX_ERR_PROC_FAILED
            MPI_Abort(world_comm, error);
        errhandler(&world_comm, &comm, &distance, src_copy, send_size, data, datatype);
    }

    MPI_Comm_set_errhandler(world_comm, // no more tolerating failure
                            MPI_ERRORS_ARE_FATAL);
    MPI_Barrier(world_comm);

    /* Active ranks send back result to inactive ones */
    if (data->active == 0)
    {
        // waiting for result
        MPI_Recv(src_copy, send_size, datatype, MPI_ANY_SOURCE, 0, world_comm, MPI_STATUS_IGNORE);
    }
    else
    {
        MPI_Comm_rank(comm, &rank);
        if (rank < data->inactive_ranks_count)
        {
            // send the result
            MPI_Send(src_copy, send_size, datatype, data->inactive_ranks[rank], 0, world_comm);
        }
    }

    memcpy(dst, src_copy, send_size * type_size);
    free(src_copy);
    free(dst_copy);
}