#!/bin/bash

# Script to start gdbserver in Docker container for remote debugging
# Usage: ./debug-in-docker.sh [args for streamget]

CONTAINER_NAME="streamget-dev"
GDBSERVER_PORT="2345"
PROGRAM="/workspace/src/streamget"

# Check if container is running
if ! docker ps | grep -q "$CONTAINER_NAME"; then
    echo "Starting Docker container..."
    docker compose up -d
    sleep 2
fi

# Build the program
echo "Building streamget..."
docker compose exec $CONTAINER_NAME bash -c "cd /workspace && make"

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Starting gdbserver on port $GDBSERVER_PORT..."
echo "Connect from VSCode or run: gdb -ex 'target remote localhost:$GDBSERVER_PORT'"
echo ""
echo "Program arguments: $@"
echo ""

# Start gdbserver with the program and any arguments passed to this script
docker compose exec $CONTAINER_NAME gdbserver :$GDBSERVER_PORT $PROGRAM "$@"
