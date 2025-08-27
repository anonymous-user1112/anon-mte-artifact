#!/bin/bash

echo "1548000" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
echo "1548000" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq

echo "2245000" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq
echo "2245000" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq

echo "2850000" > /sys/devices/system/cpu/cpu8/cpufreq/scaling_min_freq
echo "2850000" > /sys/devices/system/cpu/cpu8/cpufreq/scaling_max_freq

sizes=(1 2 4 8 16 32 64 96 128 160 192 224 256 320 384 448 512 768 1024 2048 3072 4096 8192 16384 32768)

for size in "${sizes[@]}"; do
    ./bench 0 $size 100 0 
    sleep 5
    ./bench -1 $size 100 0 
    sleep 5
    ./bench -2 $size 100 0 
    sleep 5
done

for size in "${sizes[@]}"; do
    ./bench 0 $size 100 4 
    sleep 5
    ./bench -1 $size 100 4 
    sleep 5
    ./bench -2 $size 100 4 
    sleep 5
done

for size in "${sizes[@]}"; do
    ./bench 0 $size 100 8 
    sleep 5
    ./bench -1 $size 100 8 
    sleep 5
    ./bench -2 $size 100 8 
    sleep 5
done

for size in "${sizes[@]}"; do
    ./bench_sve 0 $size 100 0 
    sleep 5
    ./bench_sve -1 $size 100 0 
    sleep 5
    ./bench_sve 1 $size 100 0 
    sleep 5
done

for size in "${sizes[@]}"; do
    ./bench_sve 0 $size 100 4 
    sleep 5
    ./bench_sve -1 $size 100 4 
    sleep 5
    ./bench_sve 1 $size 100 4 
    sleep 5
done

for size in "${sizes[@]}"; do
    ./bench_sve 0 $size 100 8 
    sleep 5
    ./bench_sve -1 $size 100 8 
    sleep 5
    ./bench_sve 1 $size 100 8 
    sleep 5
done
