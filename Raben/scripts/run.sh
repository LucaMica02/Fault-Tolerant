#!/bin/bash

mkdir -p ../out

N=$((RANDOM % (32 - 4 + 1) + 4)) # 4 - 32
DELAY=$((RANDOM % (4 - 2 + 1)+ 2)) # 2 - 3
BUF_SIZE=$((RANDOM * 65536 % (80000000 - 50000000) + 50000000)) # 50000000 - 80000000
TIMEOUT=30

(
    /usr/bin/time -f "\nTime taken: %e seconds" \
    timeout $TIMEOUT docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpiexec --with-ft ulfm --map-by :OVERSUBSCRIBE -np $N ./main $BUF_SIZE > ../out/mpi_out.txt 2>&1 
) &

(../scripts/kill.sh $DELAY > ../out/docker_out.txt 2>&1) &

wait

python3 ../scripts/check.py $N $DELAY $BUF_SIZE $TIMEOUT

rm ../out/mpi_out.txt
rm ../out/docker_out.txt

sleep 1