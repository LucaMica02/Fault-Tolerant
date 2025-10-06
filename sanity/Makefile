CC     = mpicc
CFLAGS = -Wall

# Targets
all: mpi ulfm

mpi: mpi_test.c
	$(CC) $(CFLAGS) -o mpi.exe mpi.c

ulfm: ulfm_test.c
	$(CC) $(CFLAGS) -o ulfm.exe ulfm.c

clean:
	rm -f mpi.exe ulfm.exe