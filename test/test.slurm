#!/bin/bash
#SBATCH --job-name=ulfm
#SBATCH --output=test_%j.out
#SBATCH --error=test_%j.err
#SBATCH --time=15:00:00
#SBATCH --nodes=1                # number of nodes
#SBATCH --ntasks-per-node=64     # MPI ranks per node
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=IscrC_ASCEND_0

run_tests() {
    NP=$1
    BUF_SIZE=$2

    # RD
    srun --ntasks="$NP" singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif ../src/Original/rd.exe "$BUF_SIZE" > tmp/original_rd.txt
    srun --ntasks="$NP" singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif ../src/RD/main "$BUF_SIZE" > tmp/rd.txt

    # Raben
    srun --ntasks="$NP" singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif ../src/Original/raben.exe "$BUF_SIZE" > tmp/original_raben.txt
    srun --ntasks="$NP" singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif ../src/Raben/main "$BUF_SIZE" > tmp/raben.txt

    python3 check.py
}

repeats=50

for rep in $(seq 1 $repeats); do
    echo $rep

    np=4
    max_np=64

    while [ "$np" -le "$max_np" ]; do
        buf_size=67108864   #1
        max_buf=134217728   #33554432

        while [ "$buf_size" -le "$max_buf" ]; do
            run_tests "$np" "$buf_size"

            buf_size=$((buf_size * 2))
        done

        np=$((np * 2))
    done

done
