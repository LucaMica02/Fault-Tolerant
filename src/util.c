#include <mpi.h>

/*
Write the different reduction for the different datatypes
*/

void sum_int(void *a, void *b, int n)
{
    int *ai = (int *)a;
    int *bi = (int *)b;
    for (int i = 0; i < n; i++)
    {
        ai[i] += bi[i];
    }
}

void sum_double(void *a, void *b, int n)
{
    double *ad = (double *)a;
    double *bd = (double *)b;
    for (int i = 0; i < n; i++)
    {
        ad[i] += bd[i];
    }
}

void reduction(void *a, void *b, int n, MPI_Datatype type)
{
    if (type == MPI_INT)
    {
        sum_int(a, b, n);
    }
    else if (type == MPI_DOUBLE)
    {
        sum_double(a, b, n);
    }
}