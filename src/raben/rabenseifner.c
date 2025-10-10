#include "header.h"

int allreduce_rabenseifner(const void *sbuf, void *rbuf, size_t count,
                           MPI_Datatype dtype, MPI_Op op, MPI_Comm *comm)
{
    int *rindex = NULL, *rcount = NULL, *sindex = NULL, *scount = NULL;
    int size, rank;
    MPI_Comm_size(*comm, &size);
    MPI_Comm_rank(*comm, &rank);

    // Find number of steps of scatter-reduce and allgather,
    // biggest power of two smaller or equal to size,
    // size of send_window (number of chunks to send/recv at each step)
    // and alias of the rank to be used if size != adj_size
    // Determine nearest power of two less than or equal to size
    int steps = hibit(size, (int)(sizeof(size) * CHAR_BIT) - 1);
    if (-1 == steps)
    {
        return MPI_ERR_ARG;
    }
    int adjsize = 1 << steps;

    int err = MPI_SUCCESS;
    ptrdiff_t lb, extent, gap = 0;
    MPI_Type_get_extent(dtype, &lb, &extent);
    ptrdiff_t buf_size = datatype_span(dtype, count, &gap);

    /* Temporary buffer for receiving messages */
    char *tmp_buf = NULL;
    char *tmp_buf_raw = (char *)malloc(buf_size);
    if (NULL == tmp_buf_raw)
        return MPI_ERR_UNKNOWN;
    tmp_buf = tmp_buf_raw - gap;

    if (sbuf != MPI_IN_PLACE)
    {
        err = copy_buffer((char *)sbuf, (char *)rbuf, count, dtype);
        if (MPI_SUCCESS != err)
        {
            goto cleanup_and_return;
        }
    }

    /*
     * Step 1. Reduce the number of processes to the nearest lower power of two
     * p' = 2^{\floor{\log_2 p}} by removing r = p - p' processes.
     * 1. In the first 2r processes (ranks 0 to 2r - 1), all the even ranks send
     *  the second half of the input vector to their right neighbor (rank + 1)
     *  and all the odd ranks send the first half of the input vector to their
     *  left neighbor (rank - 1).
     * 2. All 2r processes compute the reduction on their half.
     * 3. The odd ranks then send the result to their left neighbors
     *  (the even ranks).
     *
     * The even ranks (0 to 2r - 1) now contain the reduction with the input
     * vector on their right neighbors (the odd ranks). The first r even
     * processes and the p - 2r last processes are renumbered from
     * 0 to 2^{\floor{\log_2 p}} - 1.
     */

    int vrank, step, wsize;
    int nprocs_rem = size - adjsize;
    int corr = 0;

    if (rank < 2 * nprocs_rem)
    {
        int count_lhalf = count / 2;
        int count_rhalf = count - count_lhalf;

        if (rank % 2 != 0)
        {
            /*
             * Odd process -- exchange with rank - 1
             * Send the left half of the input vector to the left neighbor,
             * Recv the right half of the input vector from the left neighbor
             */
            err = MPI_Sendrecv(rbuf, count_lhalf, dtype, rank - 1, 0,
                               (char *)tmp_buf + (ptrdiff_t)count_lhalf * extent,
                               count_rhalf, dtype, rank - 1, 0, *comm, MPI_STATUS_IGNORE);
            if (MPI_SUCCESS != err)
            {
                goto cleanup_and_return;
            }

            /* Reduce on the right half of the buffers (result in rbuf) */
            MPI_Reduce_local((char *)tmp_buf + (ptrdiff_t)count_lhalf * extent,
                             (char *)rbuf + count_lhalf * extent, count_rhalf, dtype, op);

            /* Send the right half to the left neighbor */
            err = MPI_Send((char *)rbuf + (ptrdiff_t)count_lhalf * extent,
                           count_rhalf, dtype, rank - 1, 0, *comm);
            if (MPI_SUCCESS != err)
            {
                goto cleanup_and_return;
            }

            /* This process does not pariticipate in recursive doubling phase */
            vrank = -1;
        }
        else
        {
            /*
             * Even process -- exchange with rank + 1
             * Send the right half of the input vector to the right neighbor,
             * Recv the left half of the input vector from the right neighbor
             */
            err = MPI_Sendrecv((char *)rbuf + (ptrdiff_t)count_lhalf * extent,
                               count_rhalf, dtype, rank + 1, 0,
                               tmp_buf, count_lhalf, dtype, rank + 1, 0, *comm,
                               MPI_STATUS_IGNORE);
            if (MPI_SUCCESS != err)
            {
                goto cleanup_and_return;
            }

            /* Reduce on the right half of the buffers (result in rbuf) */
            MPI_Reduce_local(tmp_buf, rbuf, count_lhalf, dtype, op);

            /* Recv the right half from the right neighbor */
            err = MPI_Recv((char *)rbuf + (ptrdiff_t)count_lhalf * extent,
                           count_rhalf, dtype, rank + 1, 0, *comm, MPI_STATUS_IGNORE);
            if (MPI_SUCCESS != err)
            {
                goto cleanup_and_return;
            }

            /* Align sbuf with rbuf (needed for recovery if a fault occours) */
            err = copy_buffer((char *)rbuf, (char *)sbuf, count, dtype);
            if (MPI_SUCCESS != err)
            {
                goto cleanup_and_return;
            }
            vrank = rank / 2;
        }
    }
    else
    { /* rank >= 2 * nprocs_rem */
        vrank = rank - nprocs_rem;
    }

    /*
     * Step 2. Reduce-scatter implemented with recursive vector halving and
     * recursive distance doubling. We have p' = 2^{\floor{\log_2 p}}
     * power-of-two number of processes with new ranks (vrank) and result in rbuf.
     *
     * The even-ranked processes send the right half of their buffer to rank + 1
     * and the odd-ranked processes send the left half of their buffer to
     * rank - 1. All processes then compute the reduction between the local
     * buffer and the received buffer. In the next \log_2(p') - 1 steps, the
     * buffers are recursively halved, and the distance is doubled. At the end,
     * each of the p' processes has 1 / p' of the total reduction result.
     */
    rindex = malloc(sizeof(*rindex) * steps);
    sindex = malloc(sizeof(*sindex) * steps);
    rcount = malloc(sizeof(*rcount) * steps);
    scount = malloc(sizeof(*scount) * steps);
    if (NULL == rindex || NULL == sindex || NULL == rcount || NULL == scount)
    {
        err = MPI_ERR_UNKNOWN;
        goto cleanup_and_return;
    }

    step = 0;
    wsize = count;
    sindex[0] = rindex[0] = 0;
    MPI_Barrier(*comm);
    MPI_Comm_set_errhandler(*comm, // Tolerate failure from now
                            MPI_ERRORS_RETURN);

    for (int mask = 1; mask < adjsize; mask <<= 1)
    {
        if (vrank != -1)
        {

            /*
             * On each iteration: rindex[step] = sindex[step] -- beginning of the
             * current window. Length of the current window is storded in wsize.
             */
            int vdest = vrank ^ mask;
            /* Translate vdest virtual rank to real rank */
            int dest = (vdest < nprocs_rem) ? vdest * 2 : vdest + nprocs_rem;
            if (rank < dest)
            {
                /*
                 * Recv into the left half of the current window, send the right
                 * half of the window to the peer (perform reduce on the left
                 * half of the current window)
                 */
                rcount[step] = wsize / 2;
                scount[step] = wsize - rcount[step];
                sindex[step] = rindex[step] + rcount[step];
            }
            else
            {
                /*
                 * Recv into the right half of the current window, send the left
                 * half of the window to the peer (perform reduce on the right
                 * half of the current window)
                 */
                scount[step] = wsize / 2;
                rcount[step] = wsize - scount[step];
                rindex[step] = sindex[step] + scount[step];
            }

            /* Send part of data from the rbuf, recv into the tmp_buf */
            if (step == 0)
            {
                /* Send the whole buffer instead of just the first half just the first time */
                err = MPI_Sendrecv(rbuf, count, dtype, dest, 0,
                                   tmp_buf, count, dtype, dest, 0,
                                   *comm, MPI_STATUS_IGNORE);
                /*MPI_Request req;
                MPI_Isend(rbuf, count, dtype, dest, 0, *comm, &req);
                err = MPI_Recv(tmp_buf, count, dtype, dest, 0, *comm, MPI_STATUS_IGNORE);
                MPI_Wait(&req, MPI_STATUS_IGNORE);*/
            }
            else
            {
                err = MPI_Sendrecv(
                    (char *)rbuf + (ptrdiff_t)sindex[step] * extent, scount[step], dtype, dest, 0,
                    (char *)tmp_buf + (ptrdiff_t)rindex[step] * extent, rcount[step], dtype, dest, 0,
                    *comm, MPI_STATUS_IGNORE);
                /* MPI_Request req;
                MPI_Isend((char *)rbuf + (ptrdiff_t)sindex[step] * extent,
                    scount[step], dtype, dest, 0, *comm, &req);
                err = MPI_Recv((char *)tmp_buf + (ptrdiff_t)rindex[step] * extent,
                        rcount[step], dtype, dest, 0, *comm, MPI_STATUS_IGNORE);
                MPI_Wait(&req, MPI_STATUS_IGNORE); */
            }

            if (err == MPI_SUCCESS)
            {
                /* Local reduce: rbuf[] = tmp_buf[] <op> rbuf[] */
                MPI_Reduce_local((char *)tmp_buf + (ptrdiff_t)rindex[step] * extent,
                                 (char *)rbuf + (ptrdiff_t)rindex[step] * extent,
                                 rcount[step], dtype, op);
            }
            else
            {
                corr = 1;
            }

            /* Move the current window to the received message */
            if (step + 1 < steps)
            {
                rindex[step + 1] = rindex[step];
                sindex[step + 1] = rindex[step];
                wsize = rcount[step];
            }
        }

        /*
         * At each step we add a synchronization in order to check if someone
         * inside the communicator died, in such case we need to check first
         * if we can recovery the data lose or not, if yes we do the recovery,
         * else we need to abort the program.
         */
        int flag = 1;
        MPIX_Comm_agree(*comm, &flag); // Synchronization
        err = MPI_Barrier(*comm);      // Detect failure at the previous step
        if (err != MPI_SUCCESS)
        {
            if (err != 75)
                MPI_Abort(*comm, 1);
            errhandler_reduce_scatter(comm, sbuf, rbuf, tmp_buf, rindex, sindex, rcount, scount, count, steps, &wsize, &step, adjsize, vrank, nprocs_rem, step, corr, extent, dtype, op);

            // Calcutate again rank and vrank values
            nprocs_rem--;
            MPI_Comm_rank(*comm, &rank);
            if (rank < nprocs_rem * 2)
            {
                if (rank % 2 == 0)
                    vrank = rank / 2;
                else
                    vrank = -1;
            }
            else
            {
                vrank = rank - nprocs_rem;
            }
            corr = 0;
        }
        step++;
    }

    /*
     * Assertion: each process has 1 / p' of the total reduction result:
     * rcount[nsteps - 1] elements in the rbuf[rindex[nsteps - 1], ...].
     */

    /*
     * Step 3. Allgather by the recursive doubling algorithm.
     * Each process has 1 / p' of the total reduction result:
     * rcount[nsteps - 1] elements in the rbuf[rindex[nsteps - 1], ...].
     * All exchanges are executed in reverse order relative
     * to recursive doubling (previous step).
     */

    step = steps - 1;

    for (int mask = adjsize >> 1; mask > 0; mask >>= 1)
    {
        if (vrank != -1)
        {
            int vdest = vrank ^ mask;
            /* Translate vdest virtual rank to real rank */
            int dest = (vdest < nprocs_rem) ? vdest * 2 : vdest + nprocs_rem;
            /*
             * Send rcount[step] elements from rbuf[rindex[step]...]
             * Recv scount[step] elements to rbuf[sindex[step]...]
             */
            MPI_Sendrecv(
                (char *)rbuf + (ptrdiff_t)rindex[step] * extent, rcount[step], dtype, dest, 0,
                (char *)rbuf + (ptrdiff_t)sindex[step] * extent, scount[step], dtype, dest, 0,
                *comm, MPI_STATUS_IGNORE);
            /*MPI_Request req;
            MPI_Isend((char *)rbuf + (ptrdiff_t)rindex[step] * extent,
                    rcount[step], dtype, dest, 0, *comm, &req);
            MPI_Recv((char *)rbuf + (ptrdiff_t)sindex[step] * extent,
                    scount[step], dtype, dest, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Wait(&req, MPI_STATUS_IGNORE);*/
        }

        /*
         * At each step we add a synchronization in order to check if someone
         * inside the communicator died, in such case we need to check first
         * if we can recovery the data lose or not, if yes we do the recovery,
         * else we need to abort the program.
         */
        int flag = 1;
        MPIX_Comm_agree(*comm, &flag);
        err = MPI_Barrier(*comm);
        if (err != MPI_SUCCESS)
        {
            if (err != 75)
                MPI_Abort(*comm, 1);
            errhandler_allgather(comm, rbuf, rindex, sindex, rcount, scount, count, steps, adjsize, nprocs_rem, step, extent, dtype);

            // Calcutate again rank and vrank values
            nprocs_rem--;
            MPI_Comm_rank(*comm, &rank);
            if (rank < nprocs_rem * 2)
            {
                if (rank % 2 == 0)
                    vrank = rank / 2;
                else
                    vrank = -1;
            }
            else
            {
                vrank = rank - nprocs_rem;
            }
        }
        step--;
    }

    // Don't tolerate failure anymore
    MPI_Comm_set_errhandler(*comm,
                            MPI_ERRORS_ARE_FATAL);
    MPI_Barrier(*comm);

    /*
     * Step 4. Send total result to excluded odd ranks.
     */
    if (rank < 2 * nprocs_rem)
    {
        if (rank % 2 != 0)
        {
            /* Odd process -- recv result from rank - 1 */
            MPI_Recv(rbuf, count, dtype, rank - 1,
                     0, *comm, MPI_STATUS_IGNORE);
        }
        else
        {
            /* Even process -- send result to rank + 1 */
            MPI_Send(rbuf, count, dtype, rank + 1, 0, *comm);
            /*MPI_Request req;
            MPI_Isend(rbuf, count, dtype, rank + 1, 0, *comm, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);*/
        }
    }

cleanup_and_return:
    if (NULL != tmp_buf_raw)
        free(tmp_buf_raw);
    if (NULL != rindex)
        free(rindex);
    if (NULL != sindex)
        free(sindex);
    if (NULL != rcount)
        free(rcount);
    if (NULL != scount)
        free(scount);
    return err;
}

int test(int buf_size, int rank, int size, MPI_Comm *comm)
{
    int res;
    int *buffer;
    int *result;
    clock_t start, end;
    double difftime;

    // Allocate and init the buffers
    buffer = (int *)malloc(buf_size * sizeof(int));
    result = (int *)malloc(buf_size * sizeof(int));
    for (int i = 0; i < buf_size; i++)
    {
        buffer[i] = rank;
    }

    start = clock();
    allreduce_rabenseifner(buffer, result, buf_size, MPI_INT, MPI_SUM, comm);
    MPI_Barrier(*comm);
    end = clock();
    difftime = ((double)(end - start)) / CLOCKS_PER_SEC;

    // Calculate the result
    res = 0;
    for (int i = 0; i < buf_size; i++)
    {
        res += (result[i] % 17);
    }

    // Log the info
    printf("P: %d\n", size);
    printf("Size: %d\n", buf_size);
    printf("Time: %lf\n", difftime);
    printf("Hello from %d of %d and the result is: %d\n", rank, size, res);

    if (NULL != buffer)
        free(buffer);
    if (NULL != result)
        free(result);
    return 0;
}

int main(int argc, char *argv[])
{
    int buf_size, rank, size;
    MPI_Comm comm;
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc < 2)
    {
        printf("Error: buffer size expected\n");
        return EXIT_FAILURE;
    }
    buf_size = atoi(argv[1]);

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    test(buf_size, rank, size, &comm);
    MPI_Finalize();

    return 0;
}
