#!/bin/bash


# testmte > 1 for test mte functionality, = 0 for benchmarking sync mte, = -1 for benchmark async mte behavior, = -2 for no mte
# buffer_size = size of the buffer
# setup_teardown = 0: set up, iteration (apply MTE (testmte==0), access, madvice); = 1 iteration (set up, apply MTE (testmte==0), madvice) 
# workload = 0, benchmark is memory microbenchmarking, = 1 benchmark is memset+sha256
# workload_iteration: inner workload iteration, reported time = workload_iteration * time(microbenchmark)
# iteration: how many data points we collect for each microbenchmarking
# cpu: pin to core cpu

sizes=(16384)

for size in "${sizes[@]}"; do
    ./validation 0 $size 0 0 10 100 0
    sleep 5
    ./validation -1 $size 0 0 10 100 0
    sleep 5
    ./validation -2 $size 0 0 10 100 0
    sleep 5
    
    ./validation 0 $size 0 0 10 100 4
    sleep 5
    ./validation -1 $size 0 0 10 100 4
    sleep 5
    ./validation -2 $size 0 0 10 100 4
    sleep 5
    
    ./validation 0 $size 0 0 10 100 8
    sleep 5
    ./validation -1 $size 0 0 10 100 8
    sleep 5
    ./validation -2 $size 0 0 10 100 8
    sleep 5

done
