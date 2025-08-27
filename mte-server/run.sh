#!/bin/sh

core_start=8
# declare -a ncores=(8 32 64 128)
declare -a ncores=(8 64 128)

for ncore in "${ncores[@]}"; do
	# internal script that executes the target programs take these arguments
	export NUM_CPU_CORES=$ncore
	core_end=$(( core_start + NUM_CPU_CORES - 1))
	export LOADER="numactl -C $core_start-$core_end -m0"

	export BENCH_MTE_MODE=0
	export TEST_RESULTS_DESCRIPTION="\$NUM_CPU_CORES: $NUM_CPU_CORES,\$BENCH_MTE_MODE: $BENCH_MTE_MODE,\$LOADER: $LOADER"
	echo "\$TEST_RESULTS_DESCRIPTION $TEST_RESULTS_DESCRIPTION"
	phoronix-test-suite batch-run local/mte-server-baseline

	export BENCH_MTE_MODE=3
	export TEST_RESULTS_DESCRIPTION="\$NUM_CPU_CORES: $NUM_CPU_CORES,\$BENCH_MTE_MODE: $BENCH_MTE_MODE,\$LOADER: $LOADER"
	echo "\$TEST_RESULTS_DESCRIPTION $TEST_RESULTS_DESCRIPTION"
	phoronix-test-suite batch-run local/mte-server
done
