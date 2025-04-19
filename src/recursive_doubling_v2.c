#include "header.h"

/*
 * Recursive Doubling Allreduce implementation with Fault Tolerance that does a check
 * if the partner is still alive before to actually do the sendrecv
 */
void recursive_doubling_v2(void *src, void *dst, int send_size, MPI_Comm world_comm, MPI_Comm comm, Data *data, MPI_Datatype datatype, MPI_Op op)
{
    int rank, size, distance, error, partner, type_size;
    int send_flag, recv_flag, test_flag, counter_flag;

    MPI_Type_size(datatype, &type_size);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    error = 0;

    reduce_pow2(src, dst, send_size, world_comm, data, datatype, op); // Reduce to power of two

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
            errhandler(&world_comm, &comm, &distance, src, send_size, data, datatype);
        }

        /* Compute the right partner */
        if (data->active == 1)
            MPI_Comm_rank(comm, &rank);
        partner = rank ^ distance;

        /* Exchange the data between ranks */
        if (partner < size && data->active != 0)
        {
            /* Check if our partner is alive */
            MPI_Request requests[2];
            send_flag = 1;
            recv_flag = 0;
            test_flag = 0;
            counter_flag = 0;
            // Start non-blocking send/recv
            MPI_Isend(&send_flag, 1, MPI_INT, partner, 0, comm, &requests[0]);
            MPI_Irecv(&recv_flag, 1, MPI_INT, partner, 0, comm, &requests[1]);
            // Wait for both operations to complete
            while (test_flag == 0 && counter_flag < 10)
            {
                MPI_Testall(2, requests, &test_flag, MPI_STATUSES_IGNORE);
                counter_flag++;
                usleep(10000); // sleep for 0.01 sec
            }

            if (test_flag == 1) // your partner is still alive
            {
                MPI_Sendrecv(src, send_size, datatype, partner, 0,
                             dst, send_size, datatype, partner, 0,
                             comm, MPI_STATUS_IGNORE);
            }

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
    }

    /* Detect failure at the last step */
    error = MPI_Barrier(world_comm);
    if (error != MPI_SUCCESS)
    {
        if (error != 75) // MPIX_ERR_PROC_FAILED
            MPI_Abort(world_comm, error);
        errhandler(&world_comm, &comm, &distance, src, send_size, data, datatype);
    }

    MPI_Comm_set_errhandler(world_comm, // no more tolerating failure
                            MPI_ERRORS_ARE_FATAL);
    MPI_Barrier(world_comm);

    /* Active ranks send back result to inactive ones */
    if (data->active == 0)
    {
        // waiting for result
        MPI_Recv(src, send_size, datatype, MPI_ANY_SOURCE, 0, world_comm, MPI_STATUS_IGNORE);
    }
    else
    {
        MPI_Comm_rank(comm, &rank);
        if (rank < data->inactive_ranks_count)
        {
            // send the result
            MPI_Send(src, send_size, datatype, data->inactive_ranks[rank], 0, world_comm);
        }
    }
}