#!/bin/bash

# compile all
echo "Compiling all"
make all

rm -rf logs
mkdir -p logs

echo "Running simulation"
./bin/hive > logs/hive &

# Usage: ./bin/worker <bee_id> <life_span> <bee_time_in_hive> <bee_time_outside_hive>
./bin/worker 1 3 1 3 > logs/bee_1 &
./bin/worker 2 3 1 2 > logs/bee_2 &
./bin/worker 3 3 1 2 > logs/bee_3 &
./bin/worker 4 3 1 2 > logs/bee_4 &
./bin/worker 5 3 1 2 > logs/bee_5 &
./bin/worker 6 3 1 2 > logs/bee_6 &
./bin/worker 7 3 1 2 > logs/bee_7 &
./bin/worker 8 3 1 2 > logs/bee_8 &
./bin/worker 9 3 1 2 > logs/bee_9 &
./bin/worker 10 3 1 2 > logs/bee_10 &
./bin/worker 11 3 1 2 > logs/bee_11 &
./bin/worker 12 3 1 2 > logs/bee_12 &
./bin/worker 13 3 1 2 > logs/bee_13 &
./bin/worker 14 3 1 2 > logs/bee_14 &
./bin/worker 15 3 1 2 > logs/bee_15 &
./bin/worker 16 3 1 2 > logs/bee_16 &
./bin/worker 17 3 1 2 > logs/bee_17 &  
./bin/worker 18 3 1 2 > logs/bee_18 &


