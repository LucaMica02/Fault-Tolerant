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

    MPI_Errhandler errh;
    MPI_Comm_create_errhandler(fatal_errhandler, &errh);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, errh);
    MPI_Barrier(MPI_COMM_WORLD);

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

    MPI_Barrier(world_comm);
}