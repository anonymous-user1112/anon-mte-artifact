#!/bin/bash

declare -a cpus=(0 4 8)
declare -a mtes=(0 1 -1)
declare -a sizes=(4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216)

mkdir result_micro
for cpu in "${cpus[@]}"; do
    for mte in "${mtes[@]}"; do
        for size in "${sizes[@]}"; do
            ./micro $mte $size 20 $cpu >> micro_$cpu.txt
        done
    done  
done
mv *.txt result_micro