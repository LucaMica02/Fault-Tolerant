#!/bin/bash

N=$((RANDOM % 32 + 1)) # 1 - 32
DELAY=1  #$((RANDOM % 3 + 1)) # 1 - 3
THRESHOLD=$((RANDOM % (80 - 5 + 1) + 5)) # 5 - 80
BUF_SIZE=$((RANDOM % (20000000 - 10000000 + 1) + 10000000)) # 10000000 - 20000000
TIMEOUT=30

echo "Generated values:" > ../out/test_log.txt
echo "N = $N" >> ../out/test_log.txt
echo "DELAY = $DELAY" >> ../out/test_log.txt
echo "THRESHOLD = $THRESHOLD" >> ../out/test_log.txt
echo "BUF_SIZE = $BUF_SIZE" >> ../out/test_log.txt
echo "TIMEOUT = $TIMEOUT" >> ../out/test_log.txt

# Capture the time it takes for the run.sh script to execute
{ 
    time ./../scripts/run.sh $N $DELAY $THRESHOLD $BUF_SIZE $TIMEOUT; 
} >> ../out/test_log.txt 2>&1

python3 ../scripts/check.py