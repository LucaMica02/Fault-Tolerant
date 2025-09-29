#!/bin/bash

run_tests() {
    NP=$1
    BUF_SIZE=$2

    # RD
    mpirun -np "$NP" ./../src/Original/rd.exe "$BUF_SIZE" > tmp/original_rd.txt
    mpirun -np "$NP" ./../src/RD/main "$BUF_SIZE" > tmp/rd.txt

    # Raben
    mpirun -np "$NP" ./../src/Original/raben.exe "$BUF_SIZE" > tmp/original_raben.txt
    mpirun -np "$NP" ./../src/Raben/main "$BUF_SIZE" > tmp/raben.txt

    python3 check.py
}

np=4
max_np=8 #64

while [ "$np" -le "$max_np" ]; do
    # Inner loop: buf_size
    buf_size=1
    max_buf=2 #33554432

    while [ "$buf_size" -le "$max_buf" ]; do
        run_tests "$np" "$buf_size"

        # Double buf_size
        buf_size=$((buf_size * 2))
    done

    # Double np
    np=$((np * 2))
done