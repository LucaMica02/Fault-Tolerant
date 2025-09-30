#!/bin/bash

#SBATCH --job-name=test_fault
#SBATCH --output=test_fault_%j.out
#SBATCH --error=test_fault_%j.err
#SBATCH --time=12:00:00
#SBATCH --nodes=1               # number of nodes
#SBATCH --ntasks-per-node=64    # MPI ranks per node (Ask the max value for N)
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=IscrC_ASCEND_0

mkdir -p ../out

run_experiments() {
    local runs=$1  # Number of experiments to run
    local exe=$2
    local allreduce_type=$3
    local log_file=$4
    local np=$5

    for run in $(seq 1 "$runs"); do 
        DELAY=$((RANDOM % (3 - 2 + 1) + 2))
        read BUF_SIZE <<< $(python3 get_bs.py "$np")
        TIMEOUT=30

        echo "Generated values:" > ../out/test_log.txt
        echo "N = $np" >> ../out/test_log.txt
        echo "DELAY = $DELAY" >> ../out/test_log.txt
        echo "BUF_SIZE = $BUF_SIZE" >> ../out/test_log.txt
        echo "TIMEOUT = $TIMEOUT" >> ../out/test_log.txt

        # Capture the time it takes for the run.sh script to execute
        {
            time ./run.sh "$np" "$DELAY" "$BUF_SIZE" "$TIMEOUT" "0" "$exe"
        } >> ../out/test_log.txt 2>&1

        python3 check.py "$allreduce_type" "$log_file"

        rm -f ../out/mpi_out.txt ../out/docker_out.txt ../out/test_log.txt

        echo $np $run "No Fault"
        sleep 1
    done

    count=1
    while [ "$count" -le "$runs" ]; do
        DELAY=$((RANDOM % (3 - 2 + 1) + 2))
        read BUF_SIZE <<< $(python3 get_bs.py "$np")
        TIMEOUT=30

        echo "Generated values:" > ../out/test_log.txt
        echo "N = $np" >> ../out/test_log.txt
        echo "DELAY = $DELAY" >> ../out/test_log.txt
        echo "BUF_SIZE = $BUF_SIZE" >> ../out/test_log.txt
        echo "TIMEOUT = $TIMEOUT" >> ../out/test_log.txt

        # Capture the time it takes for the run.sh script to execute
        {
            time ./run.sh "$np" "$DELAY" "$BUF_SIZE" "$TIMEOUT" "1" "$exe"
        } >> ../out/test_log.txt 2>&1

        python3 check.py "$allreduce_type" "$log_file"
        check=$(<"../out/check.txt")

        if [[ "$check" == "True" ]]; then
            count=$((count + 1))
        fi

        rm -f ../out/mpi_out.txt ../out/docker_out.txt ../out/test_log.txt

        echo $np $count "Fault"
        sleep 1
    done
}

min_np=4
max_np=64

while [ "$min_np" -le "$max_np" ]; do
    run_experiments 10 ../src/RD/main RD ../log/log_single_RD.csv $min_np
    run_experiments 10 ../src/Raben/main Raben ../log/log_single_Raben.csv $min_np
    min_np=$((min_np * 2))
done