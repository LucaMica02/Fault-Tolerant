#!/bin/bash

DELAY=$1   # Initial delay before starting
N=$2       # Number of processes to kill

sleep $DELAY

# Get the container ID of the running MPI job
CONTAINER_ID=$(docker ps --filter "ancestor=abouteiller/mpi-ft-ulfm" --format "{{.ID}}")
echo "Target container: $CONTAINER_ID"

# Find all PIDs of the process "main" inside the container
PIDS=($(docker exec "$CONTAINER_ID" pgrep main))
echo "Collected PIDs: ${PIDS[@]}"

# Kill N random processes, one at a time
for ((i=0; i<N; i++)); do
  if [ "${#PIDS[@]}" -eq 0 ]; then
    echo "No more PIDs to kill."
    break
  fi

  RANDOM_INDEX=$((RANDOM % ${#PIDS[@]}))
  RANDOM_PID=${PIDS[$RANDOM_INDEX]}

  echo "[$((i+1))/$N] Killing PID $RANDOM_PID inside container $CONTAINER_ID"
  docker exec "$CONTAINER_ID" kill -9 "$RANDOM_PID"

  # Remove killed PID from the list
  PIDS=("${PIDS[@]:0:$RANDOM_INDEX}" "${PIDS[@]:$((RANDOM_INDEX+1))}")

  # Sleep 0.5s before next kill
  sleep 0.5
done