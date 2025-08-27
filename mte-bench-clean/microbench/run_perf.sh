#!/bin/bash

declare -a cpus=(1)
declare -a mtes=(0 1)
declare -a workloads=(1 2 3)
declare -a sizes=(33554432)

for cpu in "${cpus[@]}"; do
    for mte in "${mtes[@]}"; do
        for size in "${sizes[@]}"; do
            for workload in "${workloads[@]}"; do
                perf stat -C ${cpu} -e instructions -e cycles -e ofb_full -e l1d_cache_rd -x , -o cpu${cpu}-mte${mte}-size${size}-workload${workload}.txt --append  ./micro_single $mte $size 1000 $cpu $workload 
                perf stat -a -e instructions -e cycles -e ofb_full -e l1d_cache_rd -x , -o allcpu${cpu}-mte${mte}-size${size}-workload${workload}.txt --append  ./micro_single $mte $size 1000 $cpu $workload 
            done
        done
    done  
done
