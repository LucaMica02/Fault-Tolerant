#!/bin/bash
#SBATCH --job-name=ulfm
#SBATCH --output=ulfm_%j.out
#SBATCH --error=ulfm_%j.err
#SBATCH --time=00:05:00
#SBATCH --nodes=2               # number of nodes
#SBATCH --ntasks-per-node=4     # MPI ranks per node

# Run your ULFM MPI program (if mpi version is >= 5.x)
mpirun --with-ft=ulfm -n 8 ./ulfm.exe

# Run your ULFM MPI program inside the singularity container
mpirun --with-ft ulfm -n 8 singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif ulfm.exe

# Run your ULFM MPI program inside the singularity container
singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif mpirun --with-ft ulfm -np 8 ./ulfm.exe

# Run your ULFM MPI program inside the singularity container (multinode)
srun -n 8 singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif ./ulfm.exe