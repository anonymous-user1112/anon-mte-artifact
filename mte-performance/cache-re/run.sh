#!/bin/bash

echo "1704000" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
echo "1704000" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq

echo "2367000" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq
echo "2367000" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq

echo "2367000" > /sys/devices/system/cpu/cpu5/cpufreq/scaling_max_freq
echo "2367000" > /sys/devices/system/cpu/cpu5/cpufreq/scaling_min_freq


echo "2914000" > /sys/devices/system/cpu/cpu8/cpufreq/scaling_max_freq
echo "2914000" > /sys/devices/system/cpu/cpu8/cpufreq/scaling_min_freq


# # 510 l1
# cpu_0_l1_sizes=(16 20 24 28 32 36 40 44 48 52 56 60 64 68 72 76 80)
# for size in "${cpu_0_l1_sizes[@]}"; do
#     ./cache $size 0 25
# done > cpu_0_l1.txt

# # l2
for (( i=32; i<=768; i+=8 )); do
    ./cache $i 0 25
done > cpu_0_l2.txt

# #l3
for (( i=512; i<=16384; i+=256 )); do
    ./cache $i 0 25
done > cpu_0_l3.txt

# # 715
# cpu_4_l1_sizes=(16 20 24 28 32 36 40 44 48 52 56 60 64 68 72 76 80 84 88 92 96 100 104 108 112 116 120 124 128)
# for size in "${cpu_4_l1_sizes[@]}"; do
#     ./cache $size 4 25
# done > cpu_4_l1.txt

# l2
for (( i=32; i<=768; i+=8 )); do
    ./cache $i 4 25
done > cpu_4_l2.txt

# #l3
for (( i=512; i<=16384; i+=256 )); do
    ./cache $i 4 25
done > cpu_4_l3.txt


# # # x3
# cpu_8_l1_sizes=(16 20 24 28 32 36 40 44 48 52 56 60 64 68 72 76 80 84 88 92 96 100 104 108 112 116 120 124 128)
# for size in "${cpu_8_l1_sizes[@]}"; do
#     ./cache $size 8 25
# done > cpu_8_l1.txt

# # #l2
# for (( i=64; i<=1280; i+=8 )); do
#     ./cache $i 8 25
# done > cpu_8_l2.txt

# l3
for (( i=512; i<=16384; i+=256 )); do
    ./cache $i 8 25
done > cpu_8_l3.txt

# # cortex 510 l2

for (( i=32; i<=768; i+=4 )); do
    ./cache $i 0 25 >> cpu_0_l2_two_core.txt &
    ./cache $i 1 25 >> cpu_1_l2_two_core.txt &
    ./cache $i 2 25 >> cpu_2_l2_two_core.txt &
    ./cache $i 3 25 >> cpu_3_l2_two_core.txt &
    
    # Wait for all background processes to finish before incrementing i
    wait
done

for (( i=32; i<=768; i+=4 )); do
    ./cache $i 0 25 >> cpu_0_l2_two_core_baseline.txt 
done

# cortex 715 l2

for (( i=32; i<=768; i+=8 )); do
    ./cache $i 4 25 >> cpu_4_l2_two_core.txt &
    ./cache $i 5 25 >> cpu_5_l2_two_core.txt &
    ./cache $i 6 25 >> cpu_6_l2_two_core.txt &
    ./cache $i 7 25 >> cpu_7_l2_two_core.txt &
    
    # Wait for all background processes to finish before incrementing i
    wait
done

for (( i=32; i<=768; i+=8 )); do
    ./cache $i 4 25 >> cpu_4_l2_two_core_baseline.txt 
done