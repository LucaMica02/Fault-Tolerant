#!/bin/bash

DELAY=$1   # Initial delay before starting
N=$2       # Number of processes to kill

echo "cioa"
sleep $DELAY

for ((i=0; i<N; i++)); do
    PIDS=($(ps -u $USER -o pid,stat,cmd | grep main | awk '$2 ~ /^R/ {print $1}'))
    echo $PIDS

    if [ "${#PIDS[@]}" -eq 0 ]; then
        echo "No more PIDs to kill. Exiting."
        exit 0
    fi

    RANDOM_INDEX=$((RANDOM % ${#PIDS[@]}))
    RANDOM_PID=${PIDS[$RANDOM_INDEX]}

    echo "[$((i+1))/$N] Killing PID $RANDOM_PID"
    kill -9 "$RANDOM_PID"

    sleep 0.5
done