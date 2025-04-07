#!/bin/bash

DELAY=$1
THRESHOLD=$2

sleep $DELAY

min=1
max=100

# Get the container ID of the running MPI job
CONTAINER_ID=$(docker ps --filter "ancestor=abouteiller/mpi-ft-ulfm" --format "{{.ID}}")

# Find all PIDs of the process "main" inside the container
PIDS=($(docker exec -it $CONTAINER_ID ps aux | awk '/main/ {print $1}'))

# Iterate throught the PIDS
for PID in "${PIDS[@]}";  do 
    random_value=$((RANDOM % (max - min + 1) + min))
    #echo $PID
    if [ $random_value -lt $THRESHOLD ]; then
        echo killing..
        docker exec -it "$CONTAINER_ID" kill -9 $PID
    fi
done