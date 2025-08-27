#!/bin/bash

set -e
set -x

in=$1
declare -a nop_size=(0 1 2 4 8 16 32 64 128 256)
declare -a optlevels=(0 3)
declare -a cores=(little big cxc)
for opt in "${optlevels[@]}"
do
  for n in "${nop_size[@]}"
  do
    for core in "${cores[@]}"
    do
      out="$1_${core}_result.txt"
      echo "opt: O$opt, len(nop): $n" >> $out
      bin="$in"-"$core"-O"$opt".bin
      gcc $in.c -o $bin -march=armv8.5-a+memtag -O$opt -DNNOP=NOP$n
      sleep 3 && GLIBC_TUNABLES=glibc.mem.tagging=0 bench_runner $core ./$bin 100000000 >> $out
      sleep 3 && GLIBC_TUNABLES=glibc.mem.tagging=3 bench_runner $core ./$bin 100000000 >> $out
    done
  done
done
