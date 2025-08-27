#!/bin/bash

EXEC_DIR="./exe"


OUT_DIR="./out"
rm -rf $OUT_DIR/*
mkdir -p $OUT_DIR

# Check if the directory exists
if [ ! -d "$EXEC_DIR" ]; then
    echo "Directory $EXEC_DIR does not exist."
    exit 1
fi

declare -a cores=(1)

# Loop through all files in the directory
for core in "${cores[@]}"; do
    for exec_file in "$EXEC_DIR"/*; do
        # Check if the file is executable
        if [ -x "$exec_file" ]; then
            filename=$(basename "$exec_file" .bin)
            out="${filename}_${core}_result.txt"
            echo "Running executable: $exec_file"

            if [[ "$exec_file" == *FLUSHCACHE.bin ]]; then
                sleep 3 && GLIBC_TUNABLES=glibc.mem.tagging=0 taskset -c $core $exec_file 1000000 100 >> ./$OUT_DIR/$out  # Run the executable
            elif [[ "$exec_file" == *FLUSHCACHETAG.bin ]]; then
                sleep 3 && GLIBC_TUNABLES=glibc.mem.tagging=1 taskset -c $core $exec_file 1000000 100 >> ./$OUT_DIR/$out  # Run the executable
                sleep 3 && GLIBC_TUNABLES=glibc.mem.tagging=3 taskset -c $core $exec_file 1000000 100 >> ./$OUT_DIR/$out  # Run the executable
            else
                sleep 3 && GLIBC_TUNABLES=glibc.mem.tagging=0 taskset -c $core $exec_file 1000000 100 >> ./$OUT_DIR/$out  # Run the executable
                sleep 3 && GLIBC_TUNABLES=glibc.mem.tagging=1 taskset -c $core $exec_file 1000000 100 >> ./$OUT_DIR/$out  # Run the executable
                sleep 3 && GLIBC_TUNABLES=glibc.mem.tagging=3 taskset -c $core $exec_file 1000000 100 >> ./$OUT_DIR/$out  # Run the executable
            fi
            
        else
            echo "Skipping non-executable file: $exec_file"
        fi
    done
done
