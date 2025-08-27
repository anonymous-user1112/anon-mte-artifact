#!/bin/bash

set -e
set -x

RESULT=result.csv
ITER=5
: > "$RESULT"

parse() {
  core=$1
  type=$2
  input=$3
  (
    grep "+R1:" "$input" | awk -F: -v core="$core" -v label="$type" '{print core",sign,"label","$3","$4}'
    grep "+R2:" "$input" | awk -F: -v core="$core" -v label="$type" '{print core",verify,"label","$3","$4}'
  )
}

# dr runs separately
declare -a suites=(default mtekmod mtesig mprotsig)
declare -a cores=(1)
declare -a rsa_sizes=(rsa512 rsa1024 rsa2048 rsa3072 rsa4096)

for suite in "${suites[@]}"; do
  if [ ! -f "openssl-$suite" ]; then
      # If it doesn't exist, print an error message to standard error (stderr)
      echo "Error: Required file '$REQUIRED_FILE' not found in the current directory." >&2
      # Exit the script with a non-zero status code to indicate an error
      exit 1
  fi
done

for ((iter=1; iter<=ITER; iter++)); do
  echo "=== Iteration $iter ==="
  for core in "${cores[@]}"; do
    LOADER="taskset -c ${core}"
    for suite in "${suites[@]}"; do
      target="openssl-$suite"
      out="$suite-$core-it$iter.txt"
      sleep 3
      ${LOADER} ./$target speed -mr ${rsa_sizes[@]} 2>&1 | tee "$out"
    done

    # dynamorio suite
    suite="dr"
    target=openssl-$suite
    out="$suite-$core-it$iter.txt"
    ${LOADER} ./dynamorio/release/bin64/drrun \
      -c ./dynamorio/build/libmemtrace_custom.so \
      -binname $target -- ./$target speed -mr ${rsa_sizes[@]} 2>&1 | tee "$out"
  done
done

# Parse all results at once
for ((iter=1; iter<=ITER; iter++)); do
  for core in "${cores[@]}"; do
    for suite in "${suites[@]}" dr; do
      out="$suite-$core-it$iter.txt"
      parse "$core" "$suite" "$out"
    done
  done
done >> "$RESULT"
