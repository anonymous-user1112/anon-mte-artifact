#!/bin/bash

set -ex

RESULT=$(pwd)/polybench.data
REPS=5

declare -a cores=(1)

declare -a progs=(
"ludcmp"
"trisolv"
"lu"
"durbin"
"gramschmidt"
"cholesky"
"doitgen"
"3mm"
"mvt"
"atax"
"2mm"
"bicg"
"syr2k"
"gesummv"
"gemver"
"gemm"
"trmm"
"symm"
"syrk"
"fdtd-2d"
"jacobi-1d"
"adi"
"heat-3d"
"jacobi-2d"
"seidel-2d"
"nussinov"
"floyd-warshall"
"deriche"
"covariance"
"correlation"
)

rm -i "$RESULT"
touch "$RESULT"

echo "core,name,baseline,mte" >> "$RESULT"

DIR=gcc15_host_out

for core in "${cores[@]}"; do
  for _ in $(seq 1 $REPS); do
    for prog in "${progs[@]}"; do
      baseline_out=$(taskset -c "$core" $DIR/wasm2c-baseline/"$prog")
      sleep 5
      mte_out=$(taskset -c "$core" $DIR/wasm2c-mte/"$prog")
      sleep 5
      echo "$core,$prog,$baseline_out,$mte_out" >> "$RESULT"
    done
  done
done
