#!/bin/bash

set -e

declare -a progs=(main.o0.FLUSH.bin main.o0.bin main.o2.FLUSH.bin main.o2.bin)
declare -a cores=(1)
declare -a bufsizes=(128 256 512 1024 2048 3072 4096)

for prog in "${progs[@]}"
do
	for core in "${cores[@]}"
	do
		# out="${prog}_${core}_result.txt"
		echo "$out"
		for size in "${bufsizes[@]}"; do
			sleep 3 && taskset -c "$core" ./"$prog" "$size"
		done
	done
done
