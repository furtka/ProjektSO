#!/bin/bash

echo "Stopping simulation"
echo "Finding all running ./bin/bee processes"
PIDS=$(pgrep -f "./bin/bee")
if [ -z "$PIDS" ]; then
    echo "No running ./bin/bee processes found."
else
    echo "Found ./bin/bee processes with PIDs: $PIDS"
    echo "Sending SIGINT to all ./bin/bee processes"
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

# send sigint to all logger_server processes
echo "Finding all running ./bin/logger_server processes"
PIDS=$(pgrep -f "./bin/logger_server")
if [ -z "$PIDS" ]; then
    echo "No running ./bin/logger_server processes found."
else
    echo "Found ./bin/logger_server processes with PIDs: $PIDS"
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
