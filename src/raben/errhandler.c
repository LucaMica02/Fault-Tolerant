#include "header.h"

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