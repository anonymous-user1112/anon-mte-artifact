#!/usr/bin/env bash

aarch64-linux-gnu-gcc -static -march=armv8.5-a -O3 -o bound wasm-rt-impl.c wasm2c_rt_minwasi.c wasm-rt-static-runner.c libjpeg.wasm.c -I./dep -DWASM_RT_MEMCHECK_BOUNDS_CHECK=1 

aarch64-linux-gnu-gcc -static -march=armv8.5-a -O3 -o guard wasm-rt-impl.c wasm2c_rt_minwasi.c wasm-rt-static-runner.c libjpeg.wasm.c -I./dep -DWASM_RT_MEMCHECK_GUARD_PAGES=1

aarch64-linux-gnu-gcc -static -march=armv8.5-a+memtag -O3 -o bound_mte wasm-rt-impl.c wasm2c_rt_minwasi.c wasm-rt-static-runner.c libjpeg.wasm.c -I./dep -DWASM_RT_MEMCHECK_BOUNDS_CHECK=1 -DWASM_RT_USE_MTE=1

aarch64-linux-gnu-gcc -static -march=armv8.5-a+memtag -O3 -o guard_mte wasm-rt-impl.c wasm2c_rt_minwasi.c wasm-rt-static-runner.c libjpeg.wasm.c -I./dep -DWASM_RT_MEMCHECK_GUARD_PAGES=1 -DWASM_RT_USE_MTE=1
