#!/usr/bin/env bash
set -x

cores=(1 5)

for core in "${cores[@]}"; do
  echo "asymm" > /sys/devices/system/cpu/cpu${core}/mte_tcf_preferred
done

run_analogues() {
  SPEC_EXT="hakc"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --size=ref --tune=base --noreportable"

  for core in "${cores[@]}"; do
    # tunable turns on the tag analogs
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" -S PASS_ENV="GLIBC_TUNABLES=glibc.mem.tagging=1" int
  done

  SPEC_EXT="hakc-sfitag"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --define=WRAPCC=/home/XXXXXXXX/code/llvm-mte/tagcfi/playground/clang_lfi_wrapper.sh --size=ref --tune=base --noreportable"

  for core in "${cores[@]}"; do
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" -S PASS_ENV="GLIBC_TUNABLES=glibc.mem.tagging=1" int
  done
}

run_default() {
  SPEC_EXT="default-static"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --size=ref --tune=base --noreportable"

  for core in "${cores[@]}"; do
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" -S PASS_ENV="GLIBC_TUNABLES=glibc.mem.tagging=0" int
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" -S PASS_ENV="GLIBC_TUNABLES=glibc.mem.tagging=1" int
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" -S PASS_ENV="GLIBC_TUNABLES=glibc.mem.tagging=3" int
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" -S PASS_ENV="GLIBC_TUNABLES=glibc.mem.tagging=5" int
  done
}

run_cfi_bwd() {
  SPEC_EXT="default-custom-glibc-libstdcxx-static"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --size=ref --tune=base --noreportable"

  for core in "${cores[@]}"; do
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" int
  done

  SPEC_EXT="tagscs-static"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --size=ref --tune=base --noreportable"

  for core in "${cores[@]}"; do
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" -S PASS_ENV="NO_MTE=1" int
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" int
  done

  SPEC_EXT="ral-half-tagcheck-static"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --size=ref --tune=base --noreportable"

  for core in "${cores[@]}"; do
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" int
  done

  SPEC_EXT="ral-full-tagcheck-static"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --size=ref --tune=base --noreportable"

  for core in "${cores[@]}"; do
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" int
  done
}

run_cfi_fwd() {
  SPEC_EXT="baseline-lto"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --size=ref --tune=base --noreportable"

  for core in "${cores[@]}"; do
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" all_c all_cpp
  done


  SPEC_EXT="tagcfi-v2-static"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --size=ref --tune=base --noreportable"

  for core in "${cores[@]}"; do
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" all_cpp
  done

  SPEC_EXT="clang-ifcc-static"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --size=ref --tune=base --noreportable"

  for core in "${cores[@]}"; do
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core}" all_cpp
  done
}

run_perfstat() {
  SPEC_EXT="default-static"
  RUNSPEC_FLAGS="--config=llvm-aarch64-mte-tagcfi.cfg --ext=$SPEC_EXT --mach=default --size=ref --tune=base --iterations=1 --noreportable"

  for core in "${cores[@]}"; do
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core} perf stat -o /tmp/spec_stat_dump_${core}.txt --append" -S PASS_ENV="GLIBC_TUNABLES=glibc.mem.tagging=0" int
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core} perf stat -o /tmp/spec_stat_dump_${core}.txt --append" -S PASS_ENV="GLIBC_TUNABLES=glibc.mem.tagging=1" int
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core} perf stat -o /tmp/spec_stat_dump_${core}.txt --append" -S PASS_ENV="GLIBC_TUNABLES=glibc.mem.tagging=3" int
    runspec ${RUNSPEC_FLAGS} -S LOADER="taskset -c ${core} perf stat -o /tmp/spec_stat_dump_${core}.txt --append" -S PASS_ENV="GLIBC_TUNABLES=glibc.mem.tagging=5" int
  done
}

"$@"
