#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <mpi.h>
#include <mpi-ext.h>

#define CHUNK_SIZE 1000

/* Data struct that contain information useful for the ranks */
typedef struct
{
    int active_ranks_count;
    int *active_ranks;
    int inactive_ranks_count;
    int *inactive_ranks;
    int active;
    int original_rank;
    int original_size;
    int master;
    int dead_partner;
} Data;

/* General implementation of recursive doubling allreduce with fault tolerance */
void recursive_doubling(void *src, void *dst, int send_size, MPI_Comm world_comm, MPI_Comm comm, Data *data, MPI_Datatype datatype, MPI_Op op);

/* Standard implementation */
void recursive_doubling_v1(void *src, void *dst, int send_size, MPI_Comm comm, MPI_Datatype datatype, int partner);

/* Implementation with partner check */
void recursive_doubling_v2(void *src, void *dst, int send_size, MPI_Comm comm, MPI_Datatype datatype, int partner, Data *data);

/* Implementation that use Isend/Irecv instead of sendrecv */
void recursive_doubling_v3(void *src, void *dst, int send_size, MPI_Comm comm, MPI_Datatype datatype, int partner, int type_size, Data *data);

/* Function that manage the events where one or more rank fail */
void errhandler(MPI_Comm *pworld, MPI_Comm *pcomm, int *distance, int *src, int send_size, Data *data, MPI_Datatype datatype);

/* Detect the failure */
void detect_failure(void *src, int send_size, MPI_Comm world_comm, MPI_Comm comm, Data *data, MPI_Datatype datatype, int *distance);

/* Function that reduce the number of ranks to the closest lower power of two */
void reduce_pow2(void *src, void *dst, int send_size, MPI_Comm world_comm, Data *data, MPI_Datatype datatype, MPI_Op op);

/* Check if the given array of size n contains the target */
int contains(int *array, int target, int n);

/* Edge cases check, check if all the ranks in a block are died or corrupted, in such cases abort the comm */
void check_abort(Data *data, int *ranks_gc, int nf, int distance, MPI_Comm pworld);

/* Check if a rank in ranks is also in ranks_gc */
int is_failed(int *ranks_gc, int *ranks, int n, int m);

#endif