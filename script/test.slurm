#!/bin/bash

#SBATCH --job-name=RD
#SBATCH --output=RD_%j.out
#SBATCH --error=RD_%j.err
#SBATCH --time=00:30:00
#SBATCH --nodes=1               # number of nodes
#SBATCH --ntasks-per-node=8    # MPI ranks per node (Ask the max value for N)
#SBATCH --partition=students

mkdir -p ../out

run_experiments() {
    local runs=$1  # Number of experiments to run
    local multiple_kill=$2
    local exe=$3
    local allreduce_type=$4
    local log_file=$5

    for run in $(seq 1 "$runs"); do
        N=$((RANDOM % (8 - 4 + 1) + 4))     
        DELAY=$((RANDOM % (3 - 2 + 1) + 2))
        read MIN MAX <<< $(python3 get_bs.py "$N")
        BUF_SIZE=$((RANDOM % (MAX - MIN + 1) + MIN))
        TIMEOUT=30

        echo "Generated values:" > ../out/test_log.txt
        echo "N = $N" >> ../out/test_log.txt
        echo "DELAY = $DELAY" >> ../out/test_log.txt
        echo "BUF_SIZE = $BUF_SIZE" >> ../out/test_log.txt
        echo "TIMEOUT = $TIMEOUT" >> ../out/test_log.txt

        # Capture the time it takes for the run.sh script to execute
        {
            time ./run.sh "$N" "$DELAY" "$BUF_SIZE" "$TIMEOUT" "$multiple_kill" "$exe"
        } >> ../out/test_log.txt 2>&1

        python3 check.py "$allreduce_type" "$log_file"

        rm -f ../out/mpi_out.txt ../out/docker_out.txt ../out/test_log.txt

        echo $run
        sleep 1
    done
}

# Call the function with 100 runs
run_experiments 100 0 ../src/RD/main RD ../log/log_single_RD.csv
run_experiments 100 1 ../src/RD/main RD ../log/log_multiple_RD.csv
run_experiments 100 0 ../src/Raben/main Raben ../log/log_single_Raben.csv