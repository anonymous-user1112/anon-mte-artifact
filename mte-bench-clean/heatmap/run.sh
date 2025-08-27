#!/bin/bash

declare -a steps=(1 2 4 8 16 32 64 128 256 512 1024)
declare -a level1s=(1 2 4 8 16 32 64 128 256 512 1024 2048 4096)
declare -a level2s=(1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144)


mkdir result_warm
for level1 in "${level1s[@]}"; do
    for level2 in "${level2s[@]}"; do
        for step in "${steps[@]}"; do
            if [[ $step -lt $level1 ]]; then
                for i in $(seq 1 10); do
                    env GLIBC_TUNABLES=glibc.mem.tagging=0 ./two_level $step $level1 $level2 1 4 >> warm_base.txt
                done
                for i in $(seq 1 10); do
                    env GLIBC_TUNABLES=glibc.mem.tagging=1 ./two_level $step $level1 $level2 1 4 >> warm_async.txt
                done
            fi
        done
    done  
done
mv *.txt result_warm


mkdir result_cold
for level1 in "${level1s[@]}"; do
    for level2 in "${level2s[@]}"; do
        for step in "${steps[@]}"; do
            if [[ $step -lt $level1 ]]; then
                for i in $(seq 1 10); do
                    env GLIBC_TUNABLES=glibc.mem.tagging=0 ./two_level $step $level1 $level2 0 4 >> cold_base.txt
                done
                for i in $(seq 1 10); do
                    env GLIBC_TUNABLES=glibc.mem.tagging=1 ./two_level $step $level1 $level2 0 4 >> cold_async.txt
                done
            fi
        done
    done  
done
mv *.txt result_cold