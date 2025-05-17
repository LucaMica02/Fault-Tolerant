#!/bin/bash

for i in {1..100};
    do
        # Scalabilità su P [4 - 8 - 16 - 32] [N = 50000000 fissato]
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 4 ./main_v1 50000000 >> ../tests/v1/time_4_5.txt
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 4 ./main_v2 50000000 >> ../tests/v2/time_4_5.txt
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 8 ./main_v1 50000000 >> ../tests/v1/time_8_5.txt
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 8 ./main_v2 50000000 >> ../tests/v2/time_8_5.txt
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 16 ./main_v1 50000000 >> ../tests/v1/time_16_5.txt
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 16 ./main_v2 50000000 >> ../tests/v2/time_16_5.txt
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 32 ./main_v1 50000000 >> ../tests/v1/time_32_5.txt
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 32 ./main_v2 50000000 >> ../tests/v2/time_32_5.txt

        # Scalabilità su N [10000000 - 20000000 - 40000000 - 80000000] [P = 32 fissato]
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 32 ./main_v1 10000000 >> ../tests/v1/time_32_1.txt 
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 32 ./main_v2 10000000 >> ../tests/v2/time_32_1.txt 
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 32 ./main_v1 20000000 >> ../tests/v1/time_32_2.txt 
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 32 ./main_v2 20000000 >> ../tests/v2/time_32_2.txt 
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 32 ./main_v1 40000000 >> ../tests/v1/time_32_4.txt 
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 32 ./main_v2 40000000 >> ../tests/v2/time_32_4.txt 
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 32 ./main_v1 80000000 >> ../tests/v1/time_32_8.txt 
        docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm --map-by :OVERSUBSCRIBE -np 32 ./main_v2 80000000 >> ../tests/v2/time_32_8.txt 
        
        echo $i
    done