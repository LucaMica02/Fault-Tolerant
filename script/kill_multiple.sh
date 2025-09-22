#!/bin/bash

DELAY=$1   # Initial delay before starting
N=$2       # Number of processes to kill

sleep $DELAY

for ((i=0; i<N; i++)); do
    PIDS=($(pgrep -u $USER main))

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