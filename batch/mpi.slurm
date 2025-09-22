#!/bin/bash
#SBATCH --job-name=mpi
#SBATCH --output=mpi_%j.out
#SBATCH --error=mpi_%j.err
#SBATCH --time=00:05:00
#SBATCH --nodes=1               # number of nodes
#SBATCH --ntasks-per-node=4     # MPI ranks per node
#SBATCH --partition=students

# Launch MPI program using srun
srun --mpi=pmix ./mpi.exe