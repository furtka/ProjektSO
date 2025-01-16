#!/bin/bash

echo "Stopping simulation"
echo "Finding all running ./bin/worker processes"
PIDS=$(pgrep -f "./bin/worker")
if [ -z "$PIDS" ]; then
    echo "No running ./bin/worker processes found."
else
    echo "Found ./bin/worker processes with PIDs: $PIDS"
    echo "Sending SIGINT to all ./bin/worker processes"
    for PID in $PIDS; do
        echo "Sending SIGINT to PID: $PID"
        kill -SIGINT "$PID"
        if [ $? -eq 0 ]; then
            echo "Successfully sent SIGINT to PID: $PID"
        else
            echo "Failed to send SIGINT to PID: $PID"
        fi
    done
fi

echo "Finding all running ./bin/hive processes"
PIDS=$(pgrep -f "./bin/hive")
if [ -z "$PIDS" ]; then
    echo "No running ./bin/hive processes found."
else
    echo "Found ./bin/hive processes with PIDs: $PIDS"
    for PID in $PIDS; do
        echo "Sending SIGINT to PID: $PID"
        kill -SIGINT "$PID"
        if [ $? -eq 0 ]; then
            echo "Successfully sent SIGINT to PID: $PID"
        else
            echo "Failed to send SIGINT to PID: $PID"
        fi
    done
fi
