CC     = mpicc
CFLAGS = -Wall

# Targets
all: mpi ulfm

mpi: mpi_check.c
	$(CC) $(CFLAGS) -o mpi.exe mpi_check.c

ulfm: ulfm_check.c
	$(CC) $(CFLAGS) -o ulfm.exe ulfm_check.c

clean:
	rm -f mpi.exe ulfm.exe