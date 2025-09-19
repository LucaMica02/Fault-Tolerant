#include "header.h"

/*
 * Manage the events where one or more rank fail
 */
void errhandler(MPI_Comm *pworld, MPI_Comm *pcomm, int *distance, int *src, int send_size, Data *data, MPI_Datatype datatype)
{

    MPI_Comm new_world, new_comm, old_comm;
    int i, j, k, rank, size, nf, p, total_count, block_count, to_recv, wk_up,
        corr, err, inactive_nf, to_send_size, to_wk_up_size;
    MPI_Group group_c, group_f, group_survivors;
    int *ranks_gc, *ranks_gf, *block_masters, *to_send, *to_wk_up;

    data->master = 0;
    to_recv = -1;
    wk_up = 0;
    to_send_size = 0;
    to_wk_up_size = 0;

    MPIX_Comm_agree(*pworld, &err);  // synchronization
    MPI_Comm_set_errhandler(*pworld, // don't allow fault tolerance here
                            MPI_ERRORS_ARE_FATAL);

    /* Check which ranks are failed */
    MPIX_Comm_failure_ack(*pworld);
    MPIX_Comm_failure_get_acked(*pworld, &group_f);
    MPI_Group_size(group_f, &nf);

    MPI_Comm_rank(*pworld, &rank);
    MPI_Comm_size(*pworld, &size);

    /* Get the failed ranks */
    ranks_gf = (int *)malloc(nf * sizeof(int));
    ranks_gc = (int *)malloc(nf * sizeof(int));
    MPI_Comm_group(*pworld, &group_c);
    for (i = 0; i < nf; i++)
        ranks_gf[i] = i;
    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);

    /* Restore the world comm */
    MPIX_Comm_shrink(*pworld, &new_world);
    old_comm = *pworld;
    *pworld = new_world;

    inactive_nf = 0;
    /* Check if is failed an inactive rank */
    if (data->inactive_ranks_count > 0 && is_failed(ranks_gc, data->inactive_ranks, nf, data->inactive_ranks_count))
    {
        /* Shift inactive ranks to the left if needed */
        for (i = 0; i < data->inactive_ranks_count; i++)
        {
            if (contains(ranks_gc, data->inactive_ranks[i], nf))
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

    /* Check if an active rank is failed */
    if (is_failed(ranks_gc, data->active_ranks, nf, data->active_ranks_count))
    {
        /* Check if all the process in one block are failed */
        if ((nf - inactive_nf) >= (*distance / 2)) // we check the fault at the previous iteration
        {
            check_abort(data, ranks_gc, nf, *distance, *pworld); // EDGE CASE
        }
        nf -= inactive_nf; // real number of active ranks failed

        /* Check if we have enough inactive ranks */
        if (nf <= data->inactive_ranks_count)
        {
            /* Check if you are inactive rank that will be waked up */
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
            {
                wk_up = 1; // the last nf inactive ranks will be woke up
            }

            /* Find the master rank for every block */
            j = 0;
            block_masters = (int *)malloc((data->active_ranks_count / (*distance / 2)) * sizeof(int));
            if (data->active == 1)
                MPI_Comm_rank(*pcomm, &rank);
            /* The first alive and not corrupted rank for every block will be the master */
            for (i = 0; i < data->active_ranks_count; i++)
            {
                if (!contains(ranks_gc, data->active_ranks[i], nf + inactive_nf) &&
                    !contains(ranks_gc, data->active_ranks[i ^ (*distance / 2)], nf + inactive_nf))
                {
                    if (i == rank)
                    {
                        data->master = 1;
                    }
                    block_masters[j++] = data->active_ranks[i];
                    i = ((*distance / 2) * j) - 1;
                }
            }

            /* Keep track of the killed ranks indexes */
            int *killed_idxs = (int *)malloc(sizeof(int) * nf);
            j = 0;
            for (i = 0; i < data->active_ranks_count; i++)
            {
                if (contains(ranks_gc, data->active_ranks[i], nf + inactive_nf))
                {
                    killed_idxs[j++] = i;
                }
            }

            /* Check which ranks have to wake up an inactive or send data to a corrupted active */
            j = data->inactive_ranks_count - 1;
            for (i = 0; i < data->active_ranks_count; i++)
            {
                if (contains(ranks_gc, data->active_ranks[i], nf + inactive_nf))
                {
                    if (data->active == 1)
                    {
                        /* I have to wake up an inactive rank */
                        if (data->master == 1 && (i / (*distance / 2) == rank / (*distance / 2)))
                        {
                            to_wk_up_size++;
                            if (to_wk_up_size == 1)
                            {
                                to_wk_up = (int *)malloc(to_wk_up_size * sizeof(int));
                            }
                            else
                            {
                                to_wk_up = (int *)realloc(to_wk_up, to_wk_up_size * sizeof(int));
                            }
                            to_wk_up[to_wk_up_size - 1] = data->inactive_ranks[j];
                        }

                        /* I have to send the data to a corrupted rank */
                        corr = i ^ (*distance / 2);
                        if (!contains(killed_idxs, corr, nf) &&
                            (data->master == 1 && (corr / (*distance / 2) == rank / (*distance / 2))))
                        {
                            to_send_size++;
                            if (to_send_size == 1)
                            {
                                to_send = (int *)malloc(to_send_size * sizeof(int));
                            }
                            else
                            {
                                to_send = (int *)realloc(to_send, to_send_size * sizeof(int));
                            }
                            to_send[to_send_size - 1] = corr;
                        }

                        /* I'm a corrupted rank */
                        if (rank == corr)
                        {
                            to_recv = 1;
                        }
                    }
                    data->active_ranks[i] = data->inactive_ranks[j];
                    j--;
                }
            }
            data->inactive_ranks_count = j + 1;
            data->inactive_ranks = (int *)realloc(data->inactive_ranks, data->inactive_ranks_count * sizeof(int));
            free(block_masters);
            free(killed_idxs);
        }
        else // we don't have enough inactive ranks
        {
            p = (int)pow(2, floor(log2(data->active_ranks_count - nf))); // closest power of two
            k = (data->active_ranks_count / p);                          // reduced by k
            *distance /= k;                                              // restore distance
            j = data->inactive_ranks_count;
            data->inactive_ranks_count += data->active_ranks_count - p - nf;
            data->inactive_ranks = (int *)realloc(data->inactive_ranks, data->inactive_ranks_count * sizeof(int));
            int new_array[p];
            total_count = 0;
            block_count = 0;
            i = 0;
            /* Choose which ranks will be the actives */
            while (i < data->active_ranks_count)
            {
                if (i % (*distance * k) == 0) // reset the block_count if a new block is encountered
                    block_count = 0;
                /*
                 * Check if the rank is alive
                 * if the partner also is
                 * and if we need more rank for such block or not
                 */
                if (!contains(ranks_gc, data->active_ranks[i], nf + inactive_nf))
                {
                    if ((block_count < *distance) && !contains(ranks_gc, data->active_ranks[i ^ ((*distance * k) / 2)], nf + inactive_nf))
                    {
                        new_array[total_count++] = data->active_ranks[i]; // take it
                        block_count++;
                    }
                    else
                    {
                        data->inactive_ranks[j++] = data->active_ranks[i]; // put it on inactive
                    }
                }
                i++;
            }
            data->active_ranks_count = p;
            data->active_ranks = (int *)realloc(data->active_ranks, data->active_ranks_count * sizeof(int));
            memcpy(data->active_ranks, new_array, sizeof(int) * p);
        }
    }
    else // we don't have any active rank failed
    {
        nf = 0;
    }

    /* Create a new active comm */
    MPI_Comm_group(old_comm, &group_c);
    MPI_Group_incl(group_c, data->active_ranks_count, data->active_ranks, &group_survivors);
    MPI_Comm_create(*pworld, group_survivors, &new_comm);
    // MPIX_Comm_revoke(*pcomm);
    *pcomm = new_comm;

    /* Restore the data if necessary */
    for (i = 0; i < to_wk_up_size; i++)
    {
        err = MPI_Send(src, send_size, datatype, to_wk_up[i], 0, old_comm);
    }
    for (i = 0; i < to_send_size; i++)
    {
        err = MPI_Send(src, send_size, datatype, to_send[i], 0, *pcomm);
    }
    if (wk_up == 1)
    {
        MPI_Status status;
        err = MPI_Recv(src, send_size, datatype, MPI_ANY_SOURCE, 0, old_comm, &status);
    }
    if (to_recv == 1)
    {
        MPI_Status status;
        err = MPI_Recv(src, send_size, datatype, MPI_ANY_SOURCE, 0, *pcomm, &status);
    }

    /* Shift the ranks in active array */
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
    /* Shift the ranks in inactive array */
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

    /* Check if we are still active or not */
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

    if (to_send_size > 0)
        free(to_send);
    if (to_wk_up_size > 0)
        free(to_wk_up);
    MPI_Group_free(&group_survivors);
    MPI_Group_free(&group_c);
    MPI_Group_free(&group_f);
    free(ranks_gf);
    free(ranks_gc);
    MPI_Barrier(*pworld);
    MPI_Comm_set_errhandler(*pworld, // tolerate the failure again
                            MPI_ERRORS_RETURN);
}