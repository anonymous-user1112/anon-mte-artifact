#!/bin/bash


echo "1425000" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
echo "1425000" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq

echo "2130000" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq
echo "2130000" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq

echo "2687000" > /sys/devices/system/cpu/cpu8/cpufreq/scaling_min_freq
echo "2687000" > /sys/devices/system/cpu/cpu8/cpufreq/scaling_max_freq

CPU_0_sizes=( 4 8 16 32 48 64 128 192 256 1024 2048 3072 8192 9216 10240 11264 12288)
CPU_4_sizes=( 8 16 32 64 96 128 256 384 512 640 768 1024 2048 3072 8192 9216 10240 11264 12288)
CPU_8_sizes=( 8 16 32 64 96 128 256 384 512 640 768 1024 2048 4096 8192 9216 10240 11264 12288)

for size in "${CPU_0_sizes[@]}"; do
    ./bench 0 $size 100 0 
    sleep 5
    ./bench -1 $size 100 0 
    sleep 5
    ./bench 1 $size 100 0 
    sleep 5
done > out_bench_0.txt

for size in "${CPU_4_sizes[@]}"; do
    ./bench 0 $size 100 4 
    sleep 5
    ./bench -1 $size 100 4 
    sleep 5
    ./bench 1 $size 100 4 
    sleep 5
done > out_bench_4.txt

for size in "${CPU_8_sizes[@]}"; do
    ./bench 0 $size 100 8 
    sleep 5
    ./bench -1 $size 100 8 
    sleep 5
    ./bench 1 $size 100 8 
    sleep 5
done > out_bench_8.txt

for size in "${CPU_0_sizes[@]}"; do
    ./bench_sve 0 $size 100 0 
    sleep 5
    ./bench_sve -1 $size 100 0 
    sleep 5
    ./bench_sve 1 $size 100 0 
    sleep 5
done > out_bench_sve_0.txt

for size in "${CPU_4_sizes[@]}"; do
    ./bench_sve 0 $size 100 4 
    sleep 5
    ./bench_sve -1 $size 100 4 
    sleep 5
    ./bench_sve 1 $size 100 4 
    sleep 5
done > out_bench_sve_4.txt

for size in "${CPU_8_sizes[@]}"; do
    ./bench_sve 0 $size 100 8 
    sleep 5
    ./bench_sve -1 $size 100 8 
    sleep 5
    ./bench_sve 1 $size 100 8 
    sleep 5
done > out_bench_sve_8.txt
