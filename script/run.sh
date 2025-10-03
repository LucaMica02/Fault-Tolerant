#!/bin/bash

N=$1
DELAY=$2 
BUF_SIZE=$3
TIMEOUT=$4
KILL_VALUE=$5 
EXE_PATH=$6
KILL=$((RANDOM % (N - 1) + 1))

# Based on the KILL_VALUE, kill one, more than one or no one
if [[ "$KILL_VALUE" == "0" ]]; then 
    echo "Kill not enabled"
    KILL=0
elif [[ "$KILL_VALUE" == "1" ]]; then
    echo "Single Kill enabled"
    KILL=1
else 
    echo "Multiple Kill enabled"
    KILL=$((RANDOM % (N - 1) + 1))
fi

# Run the executable and the kill script
timeout "$TIMEOUT" singularity exec \
    -B "$HOME/local" -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif \
    mpiexec --with-ft ulfm -np $N ./$EXE_PATH $BUF_SIZE > ../out/mpi_out.txt &
    ./kill.sh "$DELAY" "$KILL" > ../out/docker_out.txt &

wait