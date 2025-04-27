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
        error = MPI_Barrier(world_comm); // Detect failure at the previous step
        if (error != MPI_SUCCESS)
        {
            if (error != 75) // MPIX_ERR_PROC_FAILED
            {
                MPI_Abort(world_comm, error);
            }
            errhandler(&world_comm, &comm, &distance, src, send_size, data, datatype);
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

        /* Compute the right partner */
        if (data->active == 1)
            MPI_Comm_rank(comm, &rank);
        partner = rank ^ distance;

        /* Exchange the data between ranks */
        if (partner < size && data->active != 0)
        {
            /* Choose the recursive doubling version to test */
            recursive_doubling_v1(src, dst, send_size, comm, datatype, partner);
            // recursive_doubling_v2(src, dst, send_size, comm, datatype, partner, data);
            //  recursive_doubling_v3(src, dst, send_size, comm, datatype, partner, type_size, data);

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
    error = MPI_Barrier(world_comm); // Detect failure at the previous step
    if (error != MPI_SUCCESS)
    {
        if (error != 75) // MPIX_ERR_PROC_FAILED
        {
            MPI_Abort(world_comm, error);
        }
        errhandler(&world_comm, &comm, &distance, dst, send_size, data, datatype);
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

/* Standard implementation */
void recursive_doubling_v1(void *src, void *dst, int send_size, MPI_Comm comm, MPI_Datatype datatype, int partner)
{
    /*MPI_Sendrecv(src, send_size, datatype, partner, 0,
                 dst, send_size, datatype, partner, 0,
                 comm, MPI_STATUS_IGNORE);*/
    MPI_Sendrecv_timeout(src, dst, send_size, comm, datatype, partner);
}

/* Implementation that does a check if the partner is still alive before to actually do the sendrecv */
void recursive_doubling_v2(void *src, void *dst, int send_size, MPI_Comm comm, MPI_Datatype datatype, int partner, Data *data)
{
    int send_flag, recv_flag, test_flag, counter_flag;

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
    else // save the rank of your partner to furher check if it's really dead
    {
        data->dead_partner = data->active_ranks[partner];
    }
}

/* Implementation that use the Isend/Irecv instead of the sendrecv */
void recursive_doubling_v3(void *src, void *dst, int send_size, MPI_Comm comm, MPI_Datatype datatype, int partner, int type_size, Data *data)
{
    int test_flag, counter_flag, chunk;

    for (int i = 0; i < send_size; i += CHUNK_SIZE)
    {
        MPI_Request requests[2];
        test_flag = 0;
        counter_flag = 0;
        chunk = (i + CHUNK_SIZE > send_size) ? send_size - i : CHUNK_SIZE;

        /*
         * Since we can't do the pointer arithmetic with void pointer, we cast the void pointer
         * to char pointer that have size 1 byte and then we add the offset * type_size
         */
        void *src_p = (char *)src + i * type_size;
        void *dst_p = (char *)dst + i * type_size;
        MPI_Isend(src_p, chunk, datatype, partner, 0, comm, &requests[0]);
        MPI_Irecv(dst_p, chunk, datatype, partner, 0, comm, &requests[1]);

        /*
         * How my network should be fast to deliver the CHUNK_SIZE?
         * we sleep 0.001 for 10 times for a total of 0.01
         * the CHUNK_SIZE is set to 1000, in the worst case the type is 8 byte
         * so the message will have 8000 byte = 64000bit size
         * since 64000 / 6400000 = 0.01 this mean that with a network of 6,4 Mbs
         * you're able to send the whole CHUNK
         * considering delays and your network speed, adjust the sleep parameter
         */
        while (test_flag == 0 && counter_flag < 10)
        {
            MPI_Testall(2, requests, &test_flag, MPI_STATUSES_IGNORE);
            counter_flag++;
            usleep(1000); // sleep for 0.001 sec
        }

        MPI_Testall(2, requests, &test_flag, MPI_STATUSES_IGNORE);
        if (test_flag == 0) // save the rank of your partner to furher check if it's really dead
        {
            data->dead_partner = data->active_ranks[partner];
        }
    }
}