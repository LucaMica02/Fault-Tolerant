#!/bin/bash

N=$1 # 1 - 16
DELAY=$2 # 1 - 5
THRESHOLD=$3 # 5 - 80
BUF_SIZE=$4 # 10000000 - 20000000
TIMEOUT=$5

timeout $TIMEOUT docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np $N ./main $BUF_SIZE > ../out/mpi_out.txt & 

./../scripts/kill.sh $DELAY $THRESHOLD > ../out/docker_out.txt &

wait