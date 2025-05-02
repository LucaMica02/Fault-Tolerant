#include "header.h"

/*
 * Main body to test the recursive doubling
 */
int main(int argc, char *argv[])
{
    clock_t start, end;
    double difftime;
    int init_flag, size, rank, res;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &init_flag);

    if (init_flag < MPI_THREAD_SERIALIZED)
    {
        fprintf(stderr, "Error: MPI does not provide MPI_THREAD_SERIALIZED support!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    Data *data = (Data *)malloc(sizeof(Data));

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

    data->original_rank = rank;
    data->original_size = size;
    data->active = 1;
    data->active_ranks_count = size;
    data->inactive_ranks_count = 0;
    data->active_ranks = (int *)malloc(sizeof(int) * data->active_ranks_count);
    data->inactive_ranks = (int *)malloc(sizeof(int));
    data->dead_partner = -1;
    for (int i = 0; i < size; i++)
    {
        data->active_ranks[i] = i;
    }

    start = clock();
    recursive_doubling(buffer, result, buf_size, MPI_COMM_WORLD, MPI_COMM_WORLD, data, MPI_INT, MPI_SUM);
    // printf("Hello from %d b_size: %d\n", data->original_rank, buf_size);
    end = clock();
    difftime = ((double)(end - start)) / CLOCKS_PER_SEC;

    res = 0;
    for (int i = 0; i < buf_size; i++)
    {
        res += (result[i] % 17);
    }
    if (rank == 0)
        printf("Time: %lf\n", difftime);
    // printf("Hello from %d of %d and the result is: %d\n", data->original_rank, data->original_size, res);
    free(data->inactive_ranks);
    free(data->active_ranks);
    free(data);
    free(buffer);
    free(result);
    // fprintf(stderr, "main1 of RD from %d\n", rank);
    MPI_Finalize();
    // fprintf(stderr, "main2 of RD from %d\n", rank);
    return 0;
}