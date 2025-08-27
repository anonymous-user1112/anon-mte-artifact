#!/bin/bash

# Define the source file
SRC="store_fence.c"

# Compiler flags
CFLAGS="-v -target aarch64-linux-gnu -march=armv8.5-a+memtag+sve -O0 -static "

# Output directory for executables
EXE_DIR="./exe"
mkdir -p $EXE_DIR

BARRIERS=("DMBST" "DMBINST" "DMBSTLD" "NoB")
NOPS=("NOP0" "NOP1" "NOP2" "NOP4" "NOP5" "NOP6" "NOP7" "NOP8" "NOP9" "NOP10" "NOP11" "NOP12" "NOP16" "NOP20" "NOP24" "NOP28" "NOP32")
FLUSHS=("NOFLUSH")

# Function to compile and run
compile_and_run() {
    BARRIER=$1
    NOP=$2
    FLUSH=$3
    EXE="$EXE_DIR/store_fence_${BARRIER}_${NOP}_${FLUSH}.bin"
    
    echo "Compiling with: $BARRIER $NOP $FLUSH"
    
    # Compile
    clang++ $CFLAGS -DBARRIER=$BARRIER -DNNOP=$NOP -DFLUSH=$FLUSH -o $EXE $SRC    
}

# Clean up previous builds
clean() {
    echo "Cleaning up previous builds..."
    rm -rf $EXE_DIR/*
}

# Main function
main() {
    clean

    for NOP in "${NOPS[@]}"; do
        compile_and_run "NoB" $NOP "NOFLUSH"
    done

    for BARRIER in "${BARRIERS[@]}"; do
        for FLUSH in "${FLUSHS[@]}"; do
            for NOP in "${NOPS[@]}"; do
                compile_and_run $BARRIER $NOP $FLUSH
            done
        done
    done

}

# Run the script
main

