#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int world_rank, world_size, err;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm child_comm;
    int errcodes[1]; // Array to store error codes from the child process

    // Spawn one child process
    err = MPI_Comm_spawn("child", MPI_ARGV_NULL, 1, MPI_INFO_NULL, 0,
                         MPI_COMM_WORLD, &child_comm, errcodes); // Use errcodes to capture child errors

    if (err != MPI_SUCCESS)
    {
        char error_string[MPI_MAX_ERROR_STRING];
        int length;
        MPI_Error_string(err, error_string, &length);
        printf("MPI_Comm_spawn failed: %s\n", error_string);
    }
    else
    {
        if (errcodes[0] != MPI_SUCCESS)
        {
            char error_string[MPI_MAX_ERROR_STRING];
            int length;
            MPI_Error_string(errcodes[0], error_string, &length);
            printf("Child process error: %s\n", error_string);
        }
        else
        {
            printf("Child process spawned successfully!\n");
        }

        int msg = 42;                                 // Message to send to child
        MPI_Send(&msg, 1, MPI_INT, 0, 0, child_comm); // Send to child

        // Receive the response from the child
        MPI_Recv(&msg, 1, MPI_INT, 0, 0, child_comm, MPI_STATUS_IGNORE);
        printf("Parent received: %d\n", msg);
    }

    fflush(stdout); // Flush stdout to ensure message is printed
    MPI_Finalize();
    return 0;
}
