#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    setenv("OMPI_MCA_coll_tuned_use_dynamic_rules", "1", 1);
    setenv("OMPI_MCA_coll_tuned_allreduce_algorithm", "2", 1); // Recursive Doubling

    int size, rank, res;
    clock_t start, end;
    double difftime;
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int buf_size = atoi(argv[1]);
    int *buffer;
    int *result;
    buffer = (int *)malloc(buf_size * sizeof(int));
    result = (int *)malloc(buf_size * sizeof(int));
    for (int i = 0; i < buf_size; i++)
    {
        buffer[i] = rank;
    }

    start = clock();
    MPI_Allreduce(buffer, result, buf_size, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    end = clock();
    difftime = ((double)(end - start)) / CLOCKS_PER_SEC;

    res = 0;
    for (int i = 0; i < buf_size; i++)
    {
        res += (result[i] % 17);
    }

    if (rank == 0)
        printf("Time: %lf\n", difftime);
    // printf("Hello from %d of %d and the result is: %d\n", rank, size, res);
    free(buffer);
    free(result);
    MPI_Finalize();
    return 0;
}