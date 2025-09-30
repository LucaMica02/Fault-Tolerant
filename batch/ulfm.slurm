#!/bin/bash
#SBATCH --job-name=ulfm
#SBATCH --output=ulfm_%j.out
#SBATCH --error=ulfm_%j.err
#SBATCH --time=00:05:00
#SBATCH --nodes=4               # number of nodes
#SBATCH --ntasks-per-node=2     # MPI ranks per node
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=IscrC_ASCEND_0

NTASKS=$(( SLURM_NNODES * SLURM_NTASKS_PER_NODE ))

srun --ntasks=8 singularity exec --network=host -B $HOME/local -B $TMPDIR:$TMPDIR -B /usr/lib64/openmpi:/usr/lib64/openmpi $HOME/local/mpi-ft-ulfm.sif ./ulfm.exe 1000
