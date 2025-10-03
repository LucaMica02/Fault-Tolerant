#!/bin/bash

DELAY=$1  
N=$2    

sleep $DELAY

# Kill up to N processes 
for ((i=0; i<N; i++)); do

    # Find all PIDs of the MPI program
    PIDS=($(ps -u $USER -o pid,stat,cmd | grep main | awk '$2 ~ /^R/ {print $1}'))
    echo $PIDS

    # If there isn't any pid just exit
    if [ "${#PIDS[@]}" -eq 0 ]; then
        echo "No more PIDs to kill. Exiting."
        exit 0
    fi

    # Pick a random PID to kill
    RANDOM_INDEX=$((RANDOM % ${#PIDS[@]}))
    RANDOM_PID=${PIDS[$RANDOM_INDEX]}

    # Kill the chosen PID
    echo "Killing PID $RANDOM_PID"
    kill -9 "$RANDOM_PID"

    sleep 0.5
done