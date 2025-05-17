#!/bin/bash

DELAY=$1

sleep 0.5

# Get the container ID of the running MPI job
CONTAINER_ID=$(docker ps --filter "ancestor=abouteiller/mpi-ft-ulfm" --format "{{.ID}}")

# Find all PIDs of the process "main" inside the container
PIDS=($(docker exec "$CONTAINER_ID" pgrep main))
echo "Collected PIDs: ${PIDS[@]}"

RANDOM_PID=${PIDS[$RANDOM % ${#PIDS[@]}]}
echo "Killing PID $RANDOM_PID inside container $CONTAINER_ID"
docker exec "$CONTAINER_ID" kill -9 "$RANDOM_PID"