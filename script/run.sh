#!/bin/bash

N=$1
DELAY=$2 
BUF_SIZE=$3
TIMEOUT=$4
MULTIPLE_KILL=$5 
EXE_PATH=$6

KILL=$((RANDOM % (N - 1) + 1))

if [[ "$MULTIPLE_KILL" == "1" ]]; then
    echo "Multiple kill enabled"
    timeout "$TIMEOUT" mpiexec --with-ft ulfm --map-by :OVERSUBSCRIBE -np "$N" \
        "./$EXE_PATH" "$BUF_SIZE" > ../out/mpi_out.txt &
    "./kill_multiple.sh" "$DELAY" "$KILL" > ../out/docker_out.txt &
else
    echo "Multiple kill disabled"
    timeout "$TIMEOUT" mpiexec --with-ft ulfm --map-by :OVERSUBSCRIBE -np "$N" \
        "./$EXE_PATH" "$BUF_SIZE" > ../out/mpi_out.txt &
    "./kill_single.sh" "$DELAY" > ../out/docker_out.txt &
fi

wait