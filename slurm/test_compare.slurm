#!/bin/bash
#SBATCH --job-name=test_compare
#SBATCH --output=test_compare_%j.out
#SBATCH --error=test_compare_%j.err
#SBATCH --time=12:00:00
#SBATCH --nodes=1                # number of nodes
#SBATCH --ntasks-per-node=64     # MPI ranks per node

mkdir -p ../out
mkdir -p ../log

run_tests() {
    NP=$1
    BUF_SIZE=$2

    # RD
    srun --ntasks="$NP" singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif ../src/original/rd.exe "$BUF_SIZE" > ../out/original_rd.txt
    srun --ntasks="$NP" singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif ../src/rd/main "$BUF_SIZE" > ../out/rd.txt

    # Raben
    srun --ntasks="$NP" singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif ../src/original/raben.exe "$BUF_SIZE" > ../out/original_raben.txt
    srun --ntasks="$NP" singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif ../src/raben/main "$BUF_SIZE" > ../out/raben.txt

    python3 ../analysis/check_compare.py
}

repeats=50

for rep in $(seq 1 $repeats); do
    echo $rep

    # Run with NP = {4, 8, 16, 32, 64}
    np=4
    max_np=64

    while [ "$np" -le "$max_np" ]; do
        # Buffer size from 4 Byte to 512 MB
        buf_size=1
        max_buf=134217728  

        while [ "$buf_size" -le "$max_buf" ]; do
            run_tests "$np" "$buf_size"

            buf_size=$((buf_size * 2))
        done

        np=$((np * 2))
    done

done
