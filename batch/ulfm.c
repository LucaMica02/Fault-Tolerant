#include <mpi.h>
#include <mpi-ext.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <signal.h>

static inline ptrdiff_t datatype_span(MPI_Datatype dtype, size_t count, ptrdiff_t *gap)
{
    if (count == 0)
    {
        *gap = 0;
        return 0; // No memory span required for zero repetitions
    }

    MPI_Aint lb, extent;
    MPI_Aint true_lb, true_extent;

    // Get extend and true extent (true extent does not include padding)
    MPI_Type_get_extent(dtype, &lb, &extent);
    MPI_Type_get_true_extent(dtype, &true_lb, &true_extent);

    *gap = true_lb; // Store the true lower bound

    return true_extent + extent * (count - 1); // Calculate the total memory span
}

static inline int hibit(int value, int start)
{
    unsigned int mask;

    /* Only look at the part that the caller wanted looking at */
    mask = value & ((1 << start) - 1);

    if (0 == mask)
    {
        return -1;
    }

    start = (8 * sizeof(int) - 1) - __builtin_clz(mask);

    return start;
}

static inline int copy_buffer(const void *input_buffer, void *output_buffer,
                              size_t count, const MPI_Datatype datatype)
{
    if ((input_buffer == NULL || output_buffer == NULL || count <= 0))
    {
        return MPI_ERR_UNKNOWN;
    }

    int datatype_size;
    MPI_Type_size(datatype, &datatype_size); // Get the size of the MPI datatype

    size_t total_size = count * (size_t)datatype_size;

    memcpy(output_buffer, input_buffer, total_size); // Perform the memory copy

    return MPI_SUCCESS;
}

int errhandler_reduce_scatter(MPI_Comm *comm, const void *sbuf, const void *rbuf, const void *tmp_buf, int *rindex, int *sindex, int *rcount, int *scount,
               int count, int steps, int *pwsize, int *pstep, int adjsize, int vrank, int nprocs_rem, int failed_step, int corr, ptrdiff_t extent, MPI_Datatype dtype, MPI_Op op)
{
    int rank, size, err, nf, original_partner, step, wsize, vdead, dead;
    int *ranks_gc, *ranks_gf, *group_ranks;
    int *r_rindex = NULL, *r_rcount = NULL, *r_sindex = NULL, *r_scount = NULL;
    MPI_Group group_f, group_c, group_surv;
    MPI_Comm new_comm;

    MPI_Comm_rank(*comm, &rank);
    MPI_Comm_size(*comm, &size);

    MPIX_Comm_agree(*comm, &err); // synchronization
    MPI_Comm_set_errhandler(*comm, // don't allow fault tolerance here
                            MPI_ERRORS_ARE_FATAL);

    /* Check which ranks are failed */
    MPIX_Comm_failure_ack(*comm);
    MPIX_Comm_failure_get_acked(*comm, &group_f);
    MPI_Group_size(group_f, &nf);

    /* Get the failed ranks */
    ranks_gf = (int *)malloc(nf * sizeof(int));
    ranks_gc = (int *)malloc(nf * sizeof(int));
    MPI_Comm_group(*comm, &group_c);
    for (int i = 0; i < nf; i++)
        ranks_gf[i] = i;
    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);

    // don't allow multiple failure
    if (nf > 1 || failed_step == 0)
        MPI_Abort(*comm, 1);
    dead = ranks_gc[0];

    // edge case: rank idle die
    int rank_idle_die = (dead < nprocs_rem * 2 && dead % 2 == 1) ? 1 : 0;
    if (rank_idle_die)
    {
        int last_rank_idle = nprocs_rem * 2 - 1;
        group_ranks = (int *)malloc((size - nf) * sizeof(int));
        int k = 0;
        for (int i = 0; i < size; i++)
        {
            if (i != last_rank_idle)
            {
                group_ranks[k++] = i;
            }
        }
        if (last_rank_idle != dead)
        {
            group_ranks[dead] = last_rank_idle;
        }
    }
    else
    {
        if (dead < nprocs_rem * 2)
        {
            if (dead % 2 == 0)
                vdead = dead / 2;
        }
        else
        {
            vdead = dead - nprocs_rem;
        }
        int v_org_partner = vdead ^ 1;
        original_partner = (v_org_partner < nprocs_rem) ? v_org_partner * 2 : v_org_partner + nprocs_rem;
        if (rank == original_partner)
        {
            rank = dead;
            vrank = vdead;
            r_rindex = malloc(sizeof(*r_rindex) * steps);
            r_sindex = malloc(sizeof(*r_sindex) * steps);
            r_rcount = malloc(sizeof(*r_rcount) * steps);
            r_scount = malloc(sizeof(*r_scount) * steps);
            memcpy(r_rindex, rindex, sizeof(*r_rindex) * steps);
            memcpy(r_sindex, sindex, sizeof(*r_sindex) * steps);
            memcpy(r_rcount, rcount, sizeof(*r_rcount) * steps);
            memcpy(r_scount, scount, sizeof(*r_scount) * steps);
        }
        adjsize = 1 << (failed_step + 1);
        step = 0;
        wsize = count;
        sindex[0] = rindex[0] = 0;
        for (int mask = 1; mask < adjsize; mask <<= 1)
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

            if (rank == dead && step != 0)
            {
                MPI_Recv((char *)tmp_buf + (ptrdiff_t)rindex[step] * extent, rcount[step], dtype, dest, 0, *comm, MPI_STATUS_IGNORE);

                /* Local reduce: sbuf[] = tmp_buf[] <op> rbuf[] */
                MPI_Reduce_local((char *)tmp_buf + (ptrdiff_t)rindex[step] * extent,
                                 (char *)sbuf + (ptrdiff_t)rindex[step] * extent,
                                 rcount[step], dtype, op);

                if (step == failed_step)
                {
                    MPI_Send((char *)sbuf + (ptrdiff_t)sindex[step] * extent, scount[step], dtype, dest, 0, *comm);
                }
            }
            else if (dest == dead && step != 0)
            {
                MPI_Request req;
                MPI_Isend((char *)rbuf + (ptrdiff_t)sindex[step] * extent, scount[step], dtype, original_partner, 0, *comm, &req);
                MPI_Wait(&req, MPI_STATUS_IGNORE);
                if (step == failed_step)
                {
                    MPI_Recv((char *)tmp_buf + (ptrdiff_t)rindex[step] * extent, rcount[step], dtype, original_partner, 0, *comm, MPI_STATUS_IGNORE);
                    if (corr)
                    {
                        /* Local reduce: rbuf[] = tmp_buf[] <op> rbuf[] */
                        MPI_Reduce_local((char *)tmp_buf + (ptrdiff_t)rindex[step] * extent,
                                         (char *)rbuf + (ptrdiff_t)rindex[step] * extent,
                                         rcount[step], dtype, op);
                    }
                }
            }

            /* Move the current window to the received message */
            if (step + 1 < steps)
            {
                rindex[step + 1] = rindex[step];
                sindex[step + 1] = rindex[step];
                wsize = rcount[step];
            }

            if (rank == dead && step == 0)
            {
                /* Local reduce: sbuf[] = tmp_buf[] <op> sbuf[] */
                MPI_Reduce_local((char *)tmp_buf + (ptrdiff_t)rindex[step] * extent,
                                 (char *)sbuf + (ptrdiff_t)rindex[step] * extent,
                                 rcount[step], dtype, op);
            }
            if ((mask << 1) < adjsize) step++;
        }

        int new_entry = (nprocs_rem * 2) - 1;
        if (new_entry == -1)
            MPI_Abort(*comm, 1);
        if (rank == dead)
        {
            MPI_Send(rindex, steps, MPI_INT, new_entry, 0, *comm);
            MPI_Send(sindex, steps, MPI_INT, new_entry, 0, *comm);
            MPI_Send(rcount, steps, MPI_INT, new_entry, 0, *comm);
            MPI_Send(scount, steps, MPI_INT, new_entry, 0, *comm);
            MPI_Send(sbuf, count, dtype, new_entry, 0, *comm);
            MPI_Send(&step, 1, MPI_INT, new_entry, 0, *comm);
            MPI_Send(&wsize, 1, MPI_INT, new_entry, 0, *comm);
            memcpy(rindex, r_rindex, sizeof(*r_rindex) * steps);
            memcpy(sindex, r_sindex, sizeof(*r_sindex) * steps);
            memcpy(rcount, r_rcount, sizeof(*r_rcount) * steps);
            memcpy(scount, r_scount, sizeof(*r_scount) * steps);

            free(r_rindex);
            free(r_sindex);
            free(r_rcount);
            free(r_scount);
        }
        else if (rank == new_entry)
        {
            MPI_Recv(rindex, steps, MPI_INT, original_partner, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Recv(sindex, steps, MPI_INT, original_partner, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Recv(rcount, steps, MPI_INT, original_partner, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Recv(scount, steps, MPI_INT, original_partner, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Recv(rbuf, count, dtype, original_partner, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Recv(pstep, 1, MPI_INT, original_partner, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Recv(pwsize, 1, MPI_INT, original_partner, 0, *comm, MPI_STATUS_IGNORE);
        }
        group_ranks = (int *)malloc((size - nf) * sizeof(int));
        int k = 0;
        for (int i = 0; i < size; i++)
        {
            if (i != new_entry)
            {
                group_ranks[k++] = i;
            }
        }
        if (dead < new_entry)
        {
            group_ranks[dead] = new_entry;
        }
        else
        {
            group_ranks[dead - 1] = new_entry;
        }
    }

    MPI_Comm nc;
    MPIX_Comm_shrink(*comm, &nc);
    MPI_Comm_group(*comm, &group_c);
    MPI_Group_incl(group_c, size - nf, group_ranks, &group_surv);
    MPI_Comm_create(nc, group_surv, &new_comm);
    *comm = new_comm;
    MPI_Comm_set_errhandler(*comm, // tolerate the failure again
                            MPI_ERRORS_RETURN);
    MPI_Barrier(*comm);
    return 0;
}

int errhandler_allgather(MPI_Comm *comm, const void *rbuf, int *rindex, int *sindex, int *rcount, int *scount,
               int count, int steps, int adjsize, int nprocs_rem, int failed_step, ptrdiff_t extent, MPI_Datatype dtype)
{
    int rank, size, err, nf, original_partner, vdead, dead;
    int *ranks_gc, *ranks_gf, *group_ranks;
    MPI_Group group_f, group_c, group_surv;
    MPI_Comm new_comm;

    MPIX_Comm_agree(*comm, &err);  // synchronization
    MPI_Comm_set_errhandler(*comm, // don't allow fault tolerance here
                            MPI_ERRORS_ARE_FATAL);

    MPI_Comm_rank(*comm, &rank);
    MPI_Comm_size(*comm, &size);

    /* Check which ranks are failed */
    MPIX_Comm_failure_ack(*comm);
    MPIX_Comm_failure_get_acked(*comm, &group_f);
    MPI_Group_size(group_f, &nf);

    /* Get the failed ranks */
    ranks_gf = (int *)malloc(nf * sizeof(int));
    ranks_gc = (int *)malloc(nf * sizeof(int));
    MPI_Comm_group(*comm, &group_c);
    for (int i = 0; i < nf; i++)
        ranks_gf[i] = i;
    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);

    /*
     * We support maximum one failure
     * We can't support failure in the first step
     * Since the steps go in reverse the first step in the allgather
     * will be steps-1, but the errhandler is called at the end of the step
     * when step-- is called, so we do the check with (steps - 1)
     */
    if (nf > 1 || failed_step == (steps - 1))
    {
        MPI_Abort(*comm, 1);
    }
    dead = ranks_gc[0];

    // edge case: rank idle die
    int rank_idle_die = (dead < nprocs_rem * 2 && dead % 2 == 1) ? 1 : 0;
    if (rank_idle_die)
    {
        int last_rank_idle = nprocs_rem * 2 - 1;
        group_ranks = (int *)malloc((size - nf) * sizeof(int));
        int k = 0;
        for (int i = 0; i < size; i++)
        {
            if (i != last_rank_idle)
            {
                group_ranks[k++] = i;
            }
        }
        if (last_rank_idle != dead)
        {
            group_ranks[dead] = last_rank_idle;
        }
    }
    else
    {
        if (dead < nprocs_rem * 2)
        {
            if (dead % 2 == 0)
                vdead = dead / 2;
            // else was idle
        }
        else
        {
            vdead = dead - nprocs_rem;
        }

        int v_org_partner = vdead ^ (adjsize >> 1);
        original_partner = (v_org_partner < nprocs_rem) ? v_org_partner * 2 : v_org_partner + nprocs_rem;
        int new_entry = (nprocs_rem * 2) - 1;

        // no rank idle available
        if (new_entry == -1)
            MPI_Abort(*comm, 1);

        // send data to new entry
        if (rank == original_partner)
        {
            // send the buffer and the offsets
            MPI_Send(rbuf, count, dtype, new_entry, 0, *comm);
            MPI_Send(rindex, steps, MPI_INT, new_entry, 0, *comm);
            MPI_Send(sindex, steps, MPI_INT, new_entry, 0, *comm);
            MPI_Send(rcount, steps, MPI_INT, new_entry, 0, *comm);
            MPI_Send(scount, steps, MPI_INT, new_entry, 0, *comm);
        }
        else if (rank == new_entry)
        {
            // recv the buffer and the offsets
            MPI_Recv(rbuf, count, dtype, original_partner, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Recv(rindex, steps, MPI_INT, original_partner, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Recv(sindex, steps, MPI_INT, original_partner, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Recv(rcount, steps, MPI_INT, original_partner, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Recv(scount, steps, MPI_INT, original_partner, 0, *comm, MPI_STATUS_IGNORE);
        }

        // recovery of corr data if necessary
        int v_last_partner = vdead ^ (1 << failed_step);
        int last_partner = (v_last_partner < nprocs_rem) ? v_last_partner * 2 : v_last_partner + nprocs_rem;
        if (rank == new_entry)
        {
            // send the data
            MPI_Send((char *)rbuf + (ptrdiff_t)rindex[failed_step] * extent,
                     rcount[failed_step], dtype, last_partner, 0, *comm);
        }
        else if (rank == last_partner)
        {
            // recv data
            MPI_Recv((char *)rbuf + (ptrdiff_t)sindex[failed_step] * extent,
                     scount[failed_step], dtype, new_entry, 0, *comm, MPI_STATUS_IGNORE);
        }

        group_ranks = (int *)malloc((size - nf) * sizeof(int));
        int k = 0;
        for (int i = 0; i < size; i++)
        {
            if (i != new_entry)
            {
                group_ranks[k++] = i;
            }
        }
        if (dead < new_entry)
        {
            group_ranks[dead] = new_entry;
        }
        else
        {
            group_ranks[dead - 1] = new_entry;
        }
    }

    MPI_Comm nc;
    MPIX_Comm_shrink(*comm, &nc);
    MPI_Comm_group(*comm, &group_c);
    MPI_Group_incl(group_c, size - nf, group_ranks, &group_surv);
    MPI_Comm_create(nc, group_surv, &new_comm);
    *comm = new_comm;

    MPI_Group_free(&group_surv);
    MPI_Group_free(&group_c);
    MPI_Group_free(&group_f);
    if (ranks_gc != NULL)
        free(ranks_gc);
    if (ranks_gf != NULL)
        free(ranks_gf);
    if (group_ranks != NULL)
        free(group_ranks);
    MPI_Comm_free(&nc);

    MPI_Comm_set_errhandler(*comm, // tolerate the failure again
                            MPI_ERRORS_RETURN);
    MPI_Barrier(*comm);
    return 0;
}

int allreduce_rabenseifner(const void *sbuf, void *rbuf, size_t count,
                           MPI_Datatype dtype, MPI_Op op, MPI_Comm *comm)
{
    int *rindex = NULL, *rcount = NULL, *sindex = NULL, *scount = NULL;
    int size, rank;
    MPI_Comm_size(*comm, &size);
    MPI_Comm_rank(*comm, &rank);

    printf("0 from %d\n", rank);

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

    printf("1 from %d\n", rank);

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

    printf("2 from %d\n", rank);

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
    MPI_Comm_set_errhandler(*comm, // tolerate failure
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
                /* Send the whole buffer */
                MPI_Request req;
                MPI_Isend(rbuf, count, dtype, dest, 0, *comm, &req);
                err = MPI_Recv(tmp_buf, count, dtype, dest, 0, *comm, MPI_STATUS_IGNORE);
                MPI_Wait(&req, MPI_STATUS_IGNORE);
            }
            else
            {
                MPI_Request req;
                MPI_Isend((char *)rbuf + (ptrdiff_t)sindex[step] * extent,
                      scount[step], dtype, dest, 0, *comm, &req);
                err = MPI_Recv((char *)tmp_buf + (ptrdiff_t)rindex[step] * extent,
                     rcount[step], dtype, dest, 0, *comm, MPI_STATUS_IGNORE);
                MPI_Wait(&req, MPI_STATUS_IGNORE);
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
        
        int flag = 1;
        MPIX_Comm_agree(*comm, &flag); // synchronization
        err = MPI_Barrier(*comm); // detect failure at the previous step
        if (err != MPI_SUCCESS)
        {
            if (err != 75)
                MPI_Abort(*comm, 1);
            errhandler_reduce_scatter(comm, sbuf, rbuf, tmp_buf, rindex, sindex, rcount, scount, count, steps, &wsize, &step, adjsize, vrank, nprocs_rem, step, corr, extent, dtype, op);
            // calcutate again rank and vrank
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

    printf("3 from %d\n", rank);

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
            MPI_Request req;
            MPI_Isend((char *)rbuf + (ptrdiff_t)rindex[step] * extent,
                      rcount[step], dtype, dest, 0, *comm, &req);
            MPI_Recv((char *)rbuf + (ptrdiff_t)sindex[step] * extent,
                     scount[step], dtype, dest, 0, *comm, MPI_STATUS_IGNORE);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
        }

        int flag = 1;
        MPIX_Comm_agree(*comm, &flag);
        err = MPI_Barrier(*comm);
        if (err != MPI_SUCCESS)
        {
            if (err != 75)
                MPI_Abort(*comm, 1);
            errhandler_allgather(comm, rbuf, rindex, sindex, rcount, scount, count, steps, adjsize, nprocs_rem, step, extent, dtype);
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

    MPI_Comm_set_errhandler(*comm, // don't tolerate failure anymore
                            MPI_ERRORS_ARE_FATAL);
    MPI_Barrier(*comm);

    printf("4 from %d\n", rank);

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
            MPI_Request req;
            MPI_Isend(rbuf, count, dtype, rank + 1, 0, *comm, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
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

    buffer = (int *)malloc(buf_size * sizeof(int));
    result = (int *)malloc(buf_size * sizeof(int));
    for (int i = 0; i < buf_size; i++)
    {
        buffer[i] = rank;
    }

    printf("-1 from %d\n", rank);

    allreduce_rabenseifner(buffer, result, buf_size, MPI_INT, MPI_SUM, comm);
    res = 0;
    for (int i = 0; i < buf_size; i++)
    {
        res += (result[i] % 17);
    }
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

    printf("-3 from %d\n", rank);
    printf("-2 from %d\n", rank);
    comm = MPI_COMM_WORLD;
    test(buf_size, rank, size, &comm);
    MPI_Finalize();

    return 0;
}
