#!/bin/bash

# compile all
echo "Compiling all"
make all

echo "Running simulation"
./bin/hive > logs &
# Usage: ./bin/worker <bee_id> <life_span> <bee_time_in_hive> <bee_time_outside_hive>\n
./bin/worker 1 3 1 3 &
./bin/worker 2 3 1 2 &
./bin/worker 3 3 1 2 &
./bin/worker 4 3 1 2 &
./bin/worker 5 3 1 2 &
./bin/worker 6 3 1 2 &
./bin/worker 7 3 1 2 &
./bin/worker 8 3 1 2 &
./bin/worker 9 3 1 2 &
./bin/worker 10 3 1 2 &
./bin/worker 11 3 1 2 &
./bin/worker 12 3 1 2 &
./bin/worker 13 3 1 2 &
./bin/worker 14 3 1 2 &
./bin/worker 15 3 1 2 &
./bin/worker 16 3 1 2 &
./bin/worker 17 3 1 2 &
./bin/worker 18 3 1 2 &


