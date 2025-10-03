#!/bin/bash

#SBATCH --job-name=test_fault
#SBATCH --output=test_fault_%j.out
#SBATCH --error=test_fault_%j.err
#SBATCH --time=12:00:00
#SBATCH --nodes=1               # number of nodes
#SBATCH --ntasks-per-node=64    # MPI ranks per node (Ask the max value for N)

# Create directories used for run the test
mkdir -p ../out
mkdir -p ../log

run_experiments() {
    local runs=$1 
    local exe=$2
    local allreduce_type=$3
    local log_file=$4
    local np=$5
    local delay=$6
    local size=$7

    # Do $run runs with no kill and keep the logs data
    for run in $(seq 1 "$runs"); do 
        TIMEOUT=30
        options=(0.8 1 1.2 1.5 1.7 2 2.2)
        DELAY=${options[$RANDOM % ${#options[@]}]}

        echo "Generated values:" > ../out/test_log.txt
        echo "N = $np" >> ../out/test_log.txt
        echo "DELAY = $DELAY" >> ../out/test_log.txt
        echo "BUF_SIZE = $size" >> ../out/test_log.txt
        echo "TIMEOUT = $TIMEOUT" >> ../out/test_log.txt

        {
            time ./../run/run_mpi.sh "$np" "$delay" "$size" "$TIMEOUT" "0" "$exe"
        } >> ../out/test_log.txt 2>&1

        python3 ../analysis/check_fault.py "$allreduce_type" "$log_file"

        rm -f ../out/mpi_out.txt ../out/docker_out.txt ../out/test_log.txt

        echo $np $run $allreduce_type "No Fault"
        sleep 1
    done

    # Do $run runs with 1 single kill and keep the logs data
    count=1
    while [ "$count" -le "$runs" ]; do
        TIMEOUT=30
        options=(0.8 1 1.2 1.5 1.7 2 2.2)
        DELAY=${options[$RANDOM % ${#options[@]}]}

        echo "Generated values:" > ../out/test_log.txt
        echo "N = $np" >> ../out/test_log.txt
        echo "DELAY = $DELAY" >> ../out/test_log.txt
        echo "BUF_SIZE = $size" >> ../out/test_log.txt
        echo "TIMEOUT = $TIMEOUT" >> ../out/test_log.txt

        {
            time ./../run/run_mpi.sh "$np" "$DELAY" "$size" "$TIMEOUT" "1" "$exe"
        } >> ../out/test_log.txt 2>&1

        python3 ../analysis/check_fault.py "$allreduce_type" "$log_file"
        check=$(<"../out/check.txt")

        # Increment the count if and only if there was a single kill and no error occurs
        if [[ "$check" == "True" ]]; then
            count=$((count + 1))
        fi

        rm -f ../out/mpi_out.txt ../out/docker_out.txt ../out/test_log.txt ../out/check.txt

        echo $np $count $allreduce_type "Fault"
        sleep 1
    done
}

run_experiments 50 ../src/rd/main RD ../log/log_single_RD_block.csv 4 2.0 187205409
run_experiments 50 ../src/raben/main Raben ../log/log_single_Raben_block.csv 4 2.0 187228224

run_experiments 50 ../src/rd/main RD ../log/log_single_RD_block.csv 8 2.0 133742920
run_experiments 50 ../src/raben/main Raben ../log/log_single_Raben_block.csv 8 2.0 133732254

run_experiments 50 ../src/rd/main RD ../log/log_single_RD_block.csv 16 2.0 110123560
run_experiments 50 ../src/raben/main Raben ../log/log_single_Raben_block.csv 16 2.0 110124421

run_experiments 50 ../src/rd/main RD ../log/log_single_RD_block.csv 32 2.0 56732120
run_experiments 50 ../src/raben/main Raben ../log/log_single_Raben_block.csv 32 1.2 63045188
