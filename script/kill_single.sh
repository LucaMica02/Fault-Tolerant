#!/bin/bash

DELAY=$1

sleep $DELAY

min=1
max=100

# Find all PIDs of the MPI program
PIDS=($(pgrep -u $USER main))
echo "Collected PIDs: ${PIDS[@]}"

# Pick a random PID to kill
RANDOM_PID=${PIDS[$RANDOM % ${#PIDS[@]}]}
echo "Killing PID $RANDOM_PID"
kill -9 "$RANDOM_PID"