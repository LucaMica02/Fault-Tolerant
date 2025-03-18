#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <mpi.h>
#include <mpi-ext.h> // Include ULFM extension

void recover_comm(MPI_Comm *comm)
{
    MPI_Comm new_comm;
    MPIX_Comm_shrink(*comm, &new_comm); // Shrink communicator, removing failed processes
    MPI_Comm_free(comm);                // Free old communicator
    *comm = new_comm;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int size, rank, res;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                            MPI_ERRORS_RETURN);
    // printf("Hello from %d of %d \n", rank, size);
    if (rank == 2)
    {
        raise(SIGKILL);
    }
    int err = MPI_Allreduce(&rank, &res, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (err != MPI_SUCCESS)
    {
        recover_comm(&comm);
        MPI_Allreduce(&rank, &res, 1, MPI_INT, MPI_SUM, comm);
    }
    printf("Hello from %d of %d and the result is: %d \n", rank, size, res);
    MPI_Finalize();
    return 0;
}