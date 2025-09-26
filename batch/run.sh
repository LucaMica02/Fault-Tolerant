#!/bin/bash

# Run your ULFM MPI program
singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif mpirun --with-ft ulfm -np 4 ./ulfm.exe 1000