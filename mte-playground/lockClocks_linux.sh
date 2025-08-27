#!/bin/bash

# === CONFIG ===
CPU_CORES=$(ls -d /sys/devices/system/cpu/cpu[0-9]*)

# Check if running as root
if [[ $EUID -ne 0 ]]; then
    echo "Please run as root (sudo $0)"
    exit 1
fi

echo "[INFO] Disabling Turbo Boost..."
if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
    echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo
elif [ -f /sys/devices/system/cpu/cpufreq/boost ]; then
    echo 0 > /sys/devices/system/cpu/cpufreq/boost
else
    echo "[WARN] No turbo control found."
fi

echo "[INFO] Calculating and setting frequency for each CPU core..."
for CORE_PATH in $CPU_CORES; do
    # Read the ORIGINAL hardware max frequency in kHz
    ORIGINAL_MAX_FREQ_KHZ=$(cat ${CORE_PATH}/cpufreq/cpuinfo_max_freq)

    # Check if max frequency was found
    if [ -z "$ORIGINAL_MAX_FREQ_KHZ" ]; then
        echo "Error: Could not determine original max frequency for $(basename $CORE_PATH). Skipping."
        continue
    fi

    # Calculate 80% of the original max frequency using integer arithmetic
    TARGET_FREQ_KHZ=$(echo "(${ORIGINAL_MAX_FREQ_KHZ} * 8) / 10" | bc)
    
    # Set the governor to 'performance'
    echo performance > ${CORE_PATH}/cpufreq/scaling_governor

    # Set minimum and maximum frequency
    echo ${TARGET_FREQ_KHZ} > ${CORE_PATH}/cpufreq/scaling_min_freq
    echo ${TARGET_FREQ_KHZ} > ${CORE_PATH}/cpufreq/scaling_max_freq

done

echo "" # Add a newline for separation
echo "[DONE] All CPU frequencies have been set."
echo ""
echo "--- CPU Frequency Summary ---"
# Dump the summary for all cores at the end
for CORE_PATH in $CPU_CORES; do
    CORE_NAME=$(basename $CORE_PATH)
    
    # Read the NEW current frequency (should match the target)
    CURRENT_FREQ_KHZ=$(cat ${CORE_PATH}/cpufreq/scaling_cur_freq)
    
    # Read the ORIGINAL max frequency again for the summary
    ORIGINAL_MAX_FREQ_KHZ=$(cat ${CORE_PATH}/cpufreq/cpuinfo_max_freq)
    
    printf "[INFO] ${CORE_NAME}: (%s/%s) kHz\n" "${CURRENT_FREQ_KHZ}" "${ORIGINAL_MAX_FREQ_KHZ}"
done
echo "---------------------------"
for CORE_PATH in $CPU_CORES; do
    if [ -f ${CORE_PATH}/mte_tcf_preferred ]; then
        echo "asymm" > ${CORE_PATH}/mte_tcf_preferred
    fi
done
grep . /sys/devices/system/cpu/*/mte_tcf_preferred
