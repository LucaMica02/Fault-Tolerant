#ifndef HEADER_H
#define HEADER_H

#include <mpi.h>
#include <mpi-ext.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <time.h>

/* General implementation of rabenseifner allreduce with fault tolerance */
int allreduce_rabenseifner(const void *sbuf, void *rbuf, size_t count,
                           MPI_Datatype dtype, MPI_Op op, MPI_Comm *comm);

/* Compute exact buffer size for MPI datatype */
ptrdiff_t datatype_span(MPI_Datatype dtype, size_t count, ptrdiff_t *gap);

/* Find most significant set bit in integer */
int hibit(int value, int start);

/* Perform memory copy of MPI datatype elements */
int copy_buffer(const void *input_buffer, void *output_buffer,
                size_t count, const MPI_Datatype datatype);

/* Function that manage the events of rank fail during the reduce-scatter phase */
int errhandler_reduce_scatter(MPI_Comm *comm, const void *sbuf, const void *rbuf, const void *tmp_buf, int *rindex, int *sindex, int *rcount, int *scount,
                              int count, int steps, int *pwsize, int *pstep, int adjsize, int vrank, int nprocs_rem, int failed_step, int corr, ptrdiff_t extent, MPI_Datatype dtype, MPI_Op op);

/* Function that manage the events of rank fail during the allgather phase */
int errhandler_allgather(MPI_Comm *comm, const void *rbuf, int *rindex, int *sindex, int *rcount, int *scount,
                         int count, int steps, int adjsize, int nprocs_rem, int failed_step, ptrdiff_t extent, MPI_Datatype dtype);

#endif