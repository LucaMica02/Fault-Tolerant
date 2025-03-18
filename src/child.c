#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    MPI_Comm parent_comm;
    MPI_Comm_get_parent(&parent_comm);

    if (parent_comm == MPI_COMM_NULL)
    {
        printf("Error: No parent process\n");
        MPI_Finalize();
        return 1;
    }

    int msg;
    MPI_Recv(&msg, 1, MPI_INT, 0, 0, parent_comm, MPI_STATUS_IGNORE);
    printf("Child received: %d\n", msg);

    msg += 10;                                     // Modify the message
    MPI_Send(&msg, 1, MPI_INT, 0, 0, parent_comm); // Send back to parent

    MPI_Finalize();
    return 0;
}
