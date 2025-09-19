#!/bin/bash

if [[ $# -ne 4 ]]; then
    echo "Usage: $0 MULTIPLE_KILL LOG_FILE ALLREDUCE_TYPE EXECUTABLE_FILE"
    exit 1
fi

mkdir -p ../out

MULTIPLE_KILL=$1
LOG_FILE=$2
ALLREDUCE_TYPE=$3
EXE=$4

N=$((RANDOM % (32 - 4 + 1) + 4)) # 4 - 32
DELAY=$((RANDOM % (3 - 2 + 1)+ 2)) # 2 - 4
read MIN MAX <<< $(python3 get_bs.py $N)
BUF_SIZE=$((RANDOM % (MAX - MIN + 1) + MIN))
TIMEOUT=30

echo "Generated values:" > ../out/test_log.txt
echo "N = $N" >> ../out/test_log.txt
echo "DELAY = $DELAY" >> ../out/test_log.txt
echo "BUF_SIZE = $BUF_SIZE" >> ../out/test_log.txt
echo "TIMEOUT = $TIMEOUT" >> ../out/test_log.txt

# Capture the time it takes for the run.sh script to execute
{ 
    time ./run.sh $N $DELAY $BUF_SIZE $TIMEOUT "$MULTIPLE_KILL" $EXE; 
} >> ../out/test_log.txt 2>&1

python3 check.py "$ALLREDUCE_TYPE" "$LOG_FILE"

rm ../out/mpi_out.txt
rm ../out/docker_out.txt
rm ../out/test_log.txt

sleep 1