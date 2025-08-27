NOTPARALLEL:

.DEFAULT_GOAL := build

SHELL := /bin/bash

CURR_USER=${USER}
CURR_PATH=${PATH}
CURR_TIME=$(shell date --iso=seconds)
PARALLEL_COUNT=$(shell nproc)
ROOT_PATH=$(shell realpath ../)

COMMON_FLAGS=-O3
CC=aarch64-linux-gnu-gcc -march=armv8.5-a+memtag

build:
	$(MAKE) build_native
	$(MAKE) build_wasm
	$(MAKE) build_wasm2c
	$(MAKE) build_wasm2c_baseline
	$(MAKE) build_wasm2c_mte

clean:
	rm -rf ./out

NATIVE_CFLAGS=$(COMMON_FLAGS) polybench/utilities/polybench.c -I polybench/utilities/ -DPOLYBENCH_TIME
NATIVE_LDFLAGS=-lm

build_native:
	mkdir -p out/native
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/solvers/ludcmp/ludcmp.c $(NATIVE_LDFLAGS) -o out/native/ludcmp
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/solvers/trisolv/trisolv.c $(NATIVE_LDFLAGS) -o out/native/trisolv
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/solvers/lu/lu.c $(NATIVE_LDFLAGS) -o out/native/lu
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/solvers/durbin/durbin.c $(NATIVE_LDFLAGS) -o out/native/durbin
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/solvers/gramschmidt/gramschmidt.c $(NATIVE_LDFLAGS) -o out/native/gramschmidt
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/solvers/cholesky/cholesky.c $(NATIVE_LDFLAGS) -o out/native/cholesky
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/kernels/doitgen/doitgen.c $(NATIVE_LDFLAGS) -o out/native/doitgen
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/kernels/3mm/3mm.c $(NATIVE_LDFLAGS) -o out/native/3mm
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/kernels/mvt/mvt.c $(NATIVE_LDFLAGS) -o out/native/mvt
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/kernels/atax/atax.c $(NATIVE_LDFLAGS) -o out/native/atax
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/kernels/2mm/2mm.c $(NATIVE_LDFLAGS) -o out/native/2mm
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/kernels/bicg/bicg.c $(NATIVE_LDFLAGS) -o out/native/bicg
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/blas/syr2k/syr2k.c $(NATIVE_LDFLAGS) -o out/native/syr2k
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/blas/gesummv/gesummv.c $(NATIVE_LDFLAGS) -o out/native/gesummv
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/blas/gemver/gemver.c $(NATIVE_LDFLAGS) -o out/native/gemver
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/blas/gemm/gemm.c $(NATIVE_LDFLAGS) -o out/native/gemm
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/blas/trmm/trmm.c $(NATIVE_LDFLAGS) -o out/native/trmm
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/blas/symm/symm.c $(NATIVE_LDFLAGS) -o out/native/symm
	$(CC) $(NATIVE_CFLAGS) polybench/linear-algebra/blas/syrk/syrk.c $(NATIVE_LDFLAGS) -o out/native/syrk
	$(CC) $(NATIVE_CFLAGS) polybench/stencils/fdtd-2d/fdtd-2d.c $(NATIVE_LDFLAGS) -o out/native/fdtd-2d
	$(CC) $(NATIVE_CFLAGS) polybench/stencils/jacobi-1d/jacobi-1d.c $(NATIVE_LDFLAGS) -o out/native/jacobi-1d
	$(CC) $(NATIVE_CFLAGS) polybench/stencils/adi/adi.c $(NATIVE_LDFLAGS) -o out/native/adi
	$(CC) $(NATIVE_CFLAGS) polybench/stencils/heat-3d/heat-3d.c $(NATIVE_LDFLAGS) -o out/native/heat-3d
	$(CC) $(NATIVE_CFLAGS) polybench/stencils/jacobi-2d/jacobi-2d.c $(NATIVE_LDFLAGS) -o out/native/jacobi-2d
	$(CC) $(NATIVE_CFLAGS) polybench/stencils/seidel-2d/seidel-2d.c $(NATIVE_LDFLAGS) -o out/native/seidel-2d
	$(CC) $(NATIVE_CFLAGS) polybench/medley/nussinov/nussinov.c $(NATIVE_LDFLAGS) -o out/native/nussinov
	$(CC) $(NATIVE_CFLAGS) polybench/medley/floyd-warshall/floyd-warshall.c $(NATIVE_LDFLAGS) -o out/native/floyd-warshall
	$(CC) $(NATIVE_CFLAGS) polybench/medley/deriche/deriche.c $(NATIVE_LDFLAGS) -o out/native/deriche
	$(CC) $(NATIVE_CFLAGS) polybench/datamining/covariance/covariance.c $(NATIVE_LDFLAGS) -o out/native/covariance
	$(CC) $(NATIVE_CFLAGS) polybench/datamining/correlation/correlation.c $(NATIVE_LDFLAGS) -o out/native/correlation

WASM_CFLAGS=$(COMMON_FLAGS) polybench/utilities/polybench.c -I polybench/utilities/ -DPOLYBENCH_TIME -D_WASI_EMULATED_PROCESS_CLOCKS
WASM_LDFLAGS=-lm -lwasi-emulated-process-clocks
WASI_SDK_PATH=$(ROOT_PATH)/mte-polybench/wasi-sdk
WASM_CC=$(WASI_SDK_PATH)/bin/clang --sysroot $(WASI_SDK_PATH)/share/wasi-sysroot/

wasi-sdk-25.0-x86_64-linux.tar.gz:
	wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-25/wasi-sdk-25.0-x86_64-linux.tar.gz

wasi-sdk-25.0-arm64-linux.tar.gz:
	wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-25/wasi-sdk-25.0-arm64-linux.tar.gz

wasi-sdk-arm64: wasi-sdk-25.0-arm64-linux.tar.gz
	mkdir -p $@
	tar -zxf $< -C $@ --strip-components 1

wasi-sdk: wasi-sdk-25.0-x86_64-linux.tar.gz
	mkdir -p $@
	tar -zxf $< -C $@ --strip-components 1

build_wasm: wasi-sdk
	mkdir -p out/wasm
	mkdir -p out/wasm/ludcmp && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/solvers/ludcmp/ludcmp.c $(WASM_LDFLAGS) -o out/wasm/ludcmp/sightglass.wasm
	mkdir -p out/wasm/trisolv && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/solvers/trisolv/trisolv.c $(WASM_LDFLAGS) -o out/wasm/trisolv/sightglass.wasm
	mkdir -p out/wasm/lu && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/solvers/lu/lu.c $(WASM_LDFLAGS) -o out/wasm/lu/sightglass.wasm
	mkdir -p out/wasm/durbin && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/solvers/durbin/durbin.c $(WASM_LDFLAGS) -o out/wasm/durbin/sightglass.wasm
	mkdir -p out/wasm/gramschmidt && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/solvers/gramschmidt/gramschmidt.c $(WASM_LDFLAGS) -o out/wasm/gramschmidt/sightglass.wasm
	mkdir -p out/wasm/cholesky && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/solvers/cholesky/cholesky.c $(WASM_LDFLAGS) -o out/wasm/cholesky/sightglass.wasm
	mkdir -p out/wasm/doitgen && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/kernels/doitgen/doitgen.c $(WASM_LDFLAGS) -o out/wasm/doitgen/sightglass.wasm
	mkdir -p out/wasm/3mm && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/kernels/3mm/3mm.c $(WASM_LDFLAGS) -o out/wasm/3mm/sightglass.wasm
	mkdir -p out/wasm/mvt && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/kernels/mvt/mvt.c $(WASM_LDFLAGS) -o out/wasm/mvt/sightglass.wasm
	mkdir -p out/wasm/atax && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/kernels/atax/atax.c $(WASM_LDFLAGS) -o out/wasm/atax/sightglass.wasm
	mkdir -p out/wasm/2mm && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/kernels/2mm/2mm.c $(WASM_LDFLAGS) -o out/wasm/2mm/sightglass.wasm
	mkdir -p out/wasm/bicg && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/kernels/bicg/bicg.c $(WASM_LDFLAGS) -o out/wasm/bicg/sightglass.wasm
	mkdir -p out/wasm/syr2k && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/blas/syr2k/syr2k.c $(WASM_LDFLAGS) -o out/wasm/syr2k/sightglass.wasm
	mkdir -p out/wasm/gesummv && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/blas/gesummv/gesummv.c $(WASM_LDFLAGS) -o out/wasm/gesummv/sightglass.wasm
	mkdir -p out/wasm/gemver && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/blas/gemver/gemver.c $(WASM_LDFLAGS) -o out/wasm/gemver/sightglass.wasm
	mkdir -p out/wasm/gemm && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/blas/gemm/gemm.c $(WASM_LDFLAGS) -o out/wasm/gemm/sightglass.wasm
	mkdir -p out/wasm/trmm && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/blas/trmm/trmm.c $(WASM_LDFLAGS) -o out/wasm/trmm/sightglass.wasm
	mkdir -p out/wasm/symm && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/blas/symm/symm.c $(WASM_LDFLAGS) -o out/wasm/symm/sightglass.wasm
	mkdir -p out/wasm/syrk && $(WASM_CC) $(WASM_CFLAGS) polybench/linear-algebra/blas/syrk/syrk.c $(WASM_LDFLAGS) -o out/wasm/syrk/sightglass.wasm
	mkdir -p out/wasm/fdtd-2d && $(WASM_CC) $(WASM_CFLAGS) polybench/stencils/fdtd-2d/fdtd-2d.c $(WASM_LDFLAGS) -o out/wasm/fdtd-2d/sightglass.wasm
	mkdir -p out/wasm/jacobi-1d && $(WASM_CC) $(WASM_CFLAGS) polybench/stencils/jacobi-1d/jacobi-1d.c $(WASM_LDFLAGS) -o out/wasm/jacobi-1d/sightglass.wasm
	mkdir -p out/wasm/adi && $(WASM_CC) $(WASM_CFLAGS) polybench/stencils/adi/adi.c $(WASM_LDFLAGS) -o out/wasm/adi/sightglass.wasm
	mkdir -p out/wasm/heat-3d && $(WASM_CC) $(WASM_CFLAGS) polybench/stencils/heat-3d/heat-3d.c $(WASM_LDFLAGS) -o out/wasm/heat-3d/sightglass.wasm
	mkdir -p out/wasm/jacobi-2d && $(WASM_CC) $(WASM_CFLAGS) polybench/stencils/jacobi-2d/jacobi-2d.c $(WASM_LDFLAGS) -o out/wasm/jacobi-2d/sightglass.wasm
	mkdir -p out/wasm/seidel-2d && $(WASM_CC) $(WASM_CFLAGS) polybench/stencils/seidel-2d/seidel-2d.c $(WASM_LDFLAGS) -o out/wasm/seidel-2d/sightglass.wasm
	mkdir -p out/wasm/nussinov && $(WASM_CC) $(WASM_CFLAGS) polybench/medley/nussinov/nussinov.c $(WASM_LDFLAGS) -o out/wasm/nussinov/sightglass.wasm
	mkdir -p out/wasm/floyd-warshall && $(WASM_CC) $(WASM_CFLAGS) polybench/medley/floyd-warshall/floyd-warshall.c $(WASM_LDFLAGS) -o out/wasm/floyd-warshall/sightglass.wasm
	mkdir -p out/wasm/deriche && $(WASM_CC) $(WASM_CFLAGS) polybench/medley/deriche/deriche.c $(WASM_LDFLAGS) -o out/wasm/deriche/sightglass.wasm
	mkdir -p out/wasm/covariance && $(WASM_CC) $(WASM_CFLAGS) polybench/datamining/covariance/covariance.c $(WASM_LDFLAGS) -o out/wasm/covariance/sightglass.wasm
	mkdir -p out/wasm/correlation && $(WASM_CC) $(WASM_CFLAGS) polybench/datamining/correlation/correlation.c $(WASM_LDFLAGS) -o out/wasm/correlation/sightglass.wasm

WASM2C=$(ROOT_PATH)/seguecg-wasm2c/build_release/wasm2c
$(WASM2C):
	cmake -S $(ROOT_PATH)/seguecg-wasm2c/ -B $(ROOT_PATH)/seguecg-wasm2c/build_release -DBUILD_TESTS=OFF
	cd $(ROOT_PATH)/seguecg-wasm2c/build_release && $(MAKE) -j $(PARALLEL_COUNT)

build_wasm2c: $(WASM2C)
	mkdir -p out/wasm2c
	mkdir -p out/wasm2c/ludcmp && $(WASM2C) out/wasm/ludcmp/sightglass.wasm -o out/wasm2c/ludcmp/sightglass.wasm.c
	mkdir -p out/wasm2c/trisolv && $(WASM2C) out/wasm/trisolv/sightglass.wasm -o out/wasm2c/trisolv/sightglass.wasm.c
	mkdir -p out/wasm2c/lu && $(WASM2C) out/wasm/lu/sightglass.wasm -o out/wasm2c/lu/sightglass.wasm.c
	mkdir -p out/wasm2c/durbin && $(WASM2C) out/wasm/durbin/sightglass.wasm -o out/wasm2c/durbin/sightglass.wasm.c
	mkdir -p out/wasm2c/gramschmidt && $(WASM2C) out/wasm/gramschmidt/sightglass.wasm -o out/wasm2c/gramschmidt/sightglass.wasm.c
	mkdir -p out/wasm2c/cholesky && $(WASM2C) out/wasm/cholesky/sightglass.wasm -o out/wasm2c/cholesky/sightglass.wasm.c
	mkdir -p out/wasm2c/doitgen && $(WASM2C) out/wasm/doitgen/sightglass.wasm -o out/wasm2c/doitgen/sightglass.wasm.c
	mkdir -p out/wasm2c/3mm && $(WASM2C) out/wasm/3mm/sightglass.wasm -o out/wasm2c/3mm/sightglass.wasm.c
	mkdir -p out/wasm2c/mvt && $(WASM2C) out/wasm/mvt/sightglass.wasm -o out/wasm2c/mvt/sightglass.wasm.c
	mkdir -p out/wasm2c/atax && $(WASM2C) out/wasm/atax/sightglass.wasm -o out/wasm2c/atax/sightglass.wasm.c
	mkdir -p out/wasm2c/2mm && $(WASM2C) out/wasm/2mm/sightglass.wasm -o out/wasm2c/2mm/sightglass.wasm.c
	mkdir -p out/wasm2c/bicg && $(WASM2C) out/wasm/bicg/sightglass.wasm -o out/wasm2c/bicg/sightglass.wasm.c
	mkdir -p out/wasm2c/syr2k && $(WASM2C) out/wasm/syr2k/sightglass.wasm -o out/wasm2c/syr2k/sightglass.wasm.c
	mkdir -p out/wasm2c/gesummv && $(WASM2C) out/wasm/gesummv/sightglass.wasm -o out/wasm2c/gesummv/sightglass.wasm.c
	mkdir -p out/wasm2c/gemver && $(WASM2C) out/wasm/gemver/sightglass.wasm -o out/wasm2c/gemver/sightglass.wasm.c
	mkdir -p out/wasm2c/gemm && $(WASM2C) out/wasm/gemm/sightglass.wasm -o out/wasm2c/gemm/sightglass.wasm.c
	mkdir -p out/wasm2c/trmm && $(WASM2C) out/wasm/trmm/sightglass.wasm -o out/wasm2c/trmm/sightglass.wasm.c
	mkdir -p out/wasm2c/symm && $(WASM2C) out/wasm/symm/sightglass.wasm -o out/wasm2c/symm/sightglass.wasm.c
	mkdir -p out/wasm2c/syrk && $(WASM2C) out/wasm/syrk/sightglass.wasm -o out/wasm2c/syrk/sightglass.wasm.c
	mkdir -p out/wasm2c/fdtd-2d && $(WASM2C) out/wasm/fdtd-2d/sightglass.wasm -o out/wasm2c/fdtd-2d/sightglass.wasm.c
	mkdir -p out/wasm2c/jacobi-1d && $(WASM2C) out/wasm/jacobi-1d/sightglass.wasm -o out/wasm2c/jacobi-1d/sightglass.wasm.c
	mkdir -p out/wasm2c/adi && $(WASM2C) out/wasm/adi/sightglass.wasm -o out/wasm2c/adi/sightglass.wasm.c
	mkdir -p out/wasm2c/heat-3d && $(WASM2C) out/wasm/heat-3d/sightglass.wasm -o out/wasm2c/heat-3d/sightglass.wasm.c
	mkdir -p out/wasm2c/jacobi-2d && $(WASM2C) out/wasm/jacobi-2d/sightglass.wasm -o out/wasm2c/jacobi-2d/sightglass.wasm.c
	mkdir -p out/wasm2c/seidel-2d && $(WASM2C) out/wasm/seidel-2d/sightglass.wasm -o out/wasm2c/seidel-2d/sightglass.wasm.c
	mkdir -p out/wasm2c/nussinov && $(WASM2C) out/wasm/nussinov/sightglass.wasm -o out/wasm2c/nussinov/sightglass.wasm.c
	mkdir -p out/wasm2c/floyd-warshall && $(WASM2C) out/wasm/floyd-warshall/sightglass.wasm -o out/wasm2c/floyd-warshall/sightglass.wasm.c
	mkdir -p out/wasm2c/deriche && $(WASM2C) out/wasm/deriche/sightglass.wasm -o out/wasm2c/deriche/sightglass.wasm.c
	mkdir -p out/wasm2c/covariance && $(WASM2C) out/wasm/covariance/sightglass.wasm -o out/wasm2c/covariance/sightglass.wasm.c
	mkdir -p out/wasm2c/correlation && $(WASM2C) out/wasm/correlation/sightglass.wasm -o out/wasm2c/correlation/sightglass.wasm.c

WASM2C_CFLAGS=$(COMMON_FLAGS) $(ROOT_PATH)/seguecg-wasm2c/wasm2c/wasm-rt-impl.c wasm2c_rt_minwasi.c wasm-rt-static-runner.c -I $(ROOT_PATH)/seguecg-wasm2c/wasm2c/ -DWASM_RT_MEMCHECK_GUARD_PAGES=1
WASM2C_LDFLAGS=-lm

build_wasm2c_baseline:
	mkdir -p out/wasm2c-baseline
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/ludcmp/ out/wasm2c/ludcmp/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/ludcmp
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/trisolv/ out/wasm2c/trisolv/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/trisolv
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/lu/ out/wasm2c/lu/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/lu
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/durbin/ out/wasm2c/durbin/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/durbin
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/gramschmidt/ out/wasm2c/gramschmidt/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/gramschmidt
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/cholesky/ out/wasm2c/cholesky/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/cholesky
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/doitgen/ out/wasm2c/doitgen/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/doitgen
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/3mm/ out/wasm2c/3mm/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/3mm
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/mvt/ out/wasm2c/mvt/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/mvt
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/atax/ out/wasm2c/atax/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/atax
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/2mm/ out/wasm2c/2mm/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/2mm
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/bicg/ out/wasm2c/bicg/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/bicg
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/syr2k/ out/wasm2c/syr2k/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/syr2k
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/gesummv/ out/wasm2c/gesummv/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/gesummv
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/gemver/ out/wasm2c/gemver/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/gemver
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/gemm/ out/wasm2c/gemm/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/gemm
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/trmm/ out/wasm2c/trmm/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/trmm
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/symm/ out/wasm2c/symm/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/symm
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/syrk/ out/wasm2c/syrk/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/syrk
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/fdtd-2d/ out/wasm2c/fdtd-2d/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/fdtd-2d
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/jacobi-1d/ out/wasm2c/jacobi-1d/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/jacobi-1d
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/adi/ out/wasm2c/adi/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/adi
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/heat-3d/ out/wasm2c/heat-3d/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/heat-3d
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/jacobi-2d/ out/wasm2c/jacobi-2d/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/jacobi-2d
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/seidel-2d/ out/wasm2c/seidel-2d/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/seidel-2d
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/nussinov/ out/wasm2c/nussinov/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/nussinov
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/floyd-warshall/ out/wasm2c/floyd-warshall/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/floyd-warshall
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/deriche/ out/wasm2c/deriche/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/deriche
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/covariance/ out/wasm2c/covariance/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/covariance
	$(CC) $(WASM2C_CFLAGS) -I out/wasm2c/correlation/ out/wasm2c/correlation/sightglass.wasm.c $(WASM2C_LDFLAGS) -o out/wasm2c-baseline/correlation


WASM2CMTE_CFLAGS=$(COMMON_FLAGS) $(ROOT_PATH)/seguecg-wasm2c/wasm2c/wasm-rt-impl.c wasm2c_rt_minwasi.c wasm-rt-static-runner.c -I $(ROOT_PATH)/seguecg-wasm2c/wasm2c/ -DWASM_RT_MEMCHECK_GUARD_PAGES=1 -DWASM_RT_USE_MTE=1
WASM2CMTE_LDFLAGS=-lm

build_wasm2c_mte:
	mkdir -p out/wasm2c-mte
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/ludcmp/ out/wasm2c/ludcmp/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/ludcmp
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/trisolv/ out/wasm2c/trisolv/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/trisolv
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/lu/ out/wasm2c/lu/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/lu
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/durbin/ out/wasm2c/durbin/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/durbin
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/gramschmidt/ out/wasm2c/gramschmidt/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/gramschmidt
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/cholesky/ out/wasm2c/cholesky/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/cholesky
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/doitgen/ out/wasm2c/doitgen/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/doitgen
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/3mm/ out/wasm2c/3mm/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/3mm
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/mvt/ out/wasm2c/mvt/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/mvt
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/atax/ out/wasm2c/atax/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/atax
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/2mm/ out/wasm2c/2mm/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/2mm
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/bicg/ out/wasm2c/bicg/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/bicg
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/syr2k/ out/wasm2c/syr2k/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/syr2k
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/gesummv/ out/wasm2c/gesummv/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/gesummv
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/gemver/ out/wasm2c/gemver/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/gemver
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/gemm/ out/wasm2c/gemm/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/gemm
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/trmm/ out/wasm2c/trmm/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/trmm
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/symm/ out/wasm2c/symm/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/symm
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/syrk/ out/wasm2c/syrk/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/syrk
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/fdtd-2d/ out/wasm2c/fdtd-2d/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/fdtd-2d
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/jacobi-1d/ out/wasm2c/jacobi-1d/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/jacobi-1d
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/adi/ out/wasm2c/adi/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/adi
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/heat-3d/ out/wasm2c/heat-3d/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/heat-3d
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/jacobi-2d/ out/wasm2c/jacobi-2d/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/jacobi-2d
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/seidel-2d/ out/wasm2c/seidel-2d/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/seidel-2d
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/nussinov/ out/wasm2c/nussinov/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/nussinov
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/floyd-warshall/ out/wasm2c/floyd-warshall/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/floyd-warshall
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/deriche/ out/wasm2c/deriche/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/deriche
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/covariance/ out/wasm2c/covariance/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/covariance
	$(CC) $(WASM2CMTE_CFLAGS) -I out/wasm2c/correlation/ out/wasm2c/correlation/sightglass.wasm.c $(WASM2CMTE_LDFLAGS) -o out/wasm2c-mte/correlation
