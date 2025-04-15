#!/bin/bash

N=$((RANDOM % (16 - 4 + 1) + 4)) # 4 - 16
DELAY=$((RANDOM % (4 - 2 + 1)+ 2)) # 2 - 4
THRESHOLD=$(python3 ../scripts/get_threshold.py $N)
read MIN MAX <<< $(python3 ../scripts/get_bs.py $N)
BUF_SIZE=1   #$((RANDOM % (MAX - MIN + 1) + MIN))
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