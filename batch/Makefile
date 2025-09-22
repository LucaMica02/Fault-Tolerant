CC     = mpicc
CFLAGS = -Wall

# Targets
all: mpi ulfm

mpi: mpi.c
	$(CC) $(CFLAGS) -o mpi.exe mpi.c

ulfm: ulfm.c
	$(CC) $(CFLAGS) -o ulfm.exe ulfm.c

clean:
	rm -f mpi.exe ulfm.exe