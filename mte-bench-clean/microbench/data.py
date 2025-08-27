import os
from collections import defaultdict
import numpy as np
import matplotlib.pyplot as plt
import re

def wrap_to_signed(value):
    return np.int32(np.uint32(value))


def read_file(cpu):
    fn = f"result_micro/micro_{cpu}.txt"
    
    if not os.path.exists(fn):
        print(f"File {fn} not found")
        return None

    data = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))

    workload_set = set()
    curr_size = 0
    curr_mte = -2
    with open(fn) as f:
        for line in f:
            keys = line.split(",")
            if len(keys) == 4:
                curr_size = wrap_to_signed(int(keys[1]))
                curr_mte = wrap_to_signed(int(keys[0]))
            elif len(line) > 20:
                line = line.replace(" (MTE)", "")
                curr_list = line.split(" ")
                number = curr_list[-2]
                name = " ".join(curr_list[:-2])
                data[curr_mte][name][curr_size].append(int(number))     
                workload_set.add(name)
    return data, workload_set


def extract_cpus_from_bash_script(script_path):
    with open(script_path, 'r') as file:
        for line in file:
            if 'declare -a cpus=' in line:
                # Extract the content between parentheses
                content = line.split('(')[1].split(')')[0]
                # Split by space and convert to integers
                cpus = [int(num) for num in content.split()]
                return cpus
    return None


def plot(cpu, data, workload_set):

    # workload -> size -> mean, std
    async_data = {}
    sync_data = {}
    for work in workload_set:
        async_data[work] = defaultdict(list)
        sync_data[work] = defaultdict(list)

    for work in workload_set:
        if work in data[0]:
            for size in data[0][work]:
                curr_async = data[-1][work][size]
                curr_base = data[0][work][size]
                curr_sync = data[1][work][size]
                async_ratio = [a / b if b != 0 else float('nan') for a, b in zip(curr_async, curr_base)]
                sync_ratio = [a / b if b != 0 else float('nan') for a, b in zip(curr_sync, curr_base)]
                async_data[work][size].append((np.mean(async_ratio), np.std(async_ratio)))
                sync_data[work][size].append((np.mean(sync_ratio), np.std(sync_ratio)))
    
    for work in workload_set:
        if work not in data[0]:
            for size in data[-1][work]:
                curr_async = data[-1][work][size]
                work_sync = work.replace(" (DSB MTE)", "").replace(" (DMB MTE)", "")
                # print(data[1][work])
                curr_sync = data[1][work_sync][size]
                # print(curr_sync)
                sync_ratio = [a / b if b != 0 else float('nan') for a, b in zip(curr_sync, curr_async)]
                sync_data[work][size].append((np.mean(sync_ratio), np.std(sync_ratio)))

    for work in workload_set:
        if work in data[0]:
            sizes_base = sorted(data[0][work].keys())
            means_async = [async_data[work][size][0][0] for size in sizes_base]
            stds_async = [async_data[work][size][0][1] for size in sizes_base]
            means_sync = [sync_data[work][size][0][0] for size in sizes_base]
            stds_sync = [sync_data[work][size][0][1] for size in sizes_base]

            plt.figure(figsize=(10, 6))
            plt.errorbar(sizes_base, means_async, yerr=stds_async, marker='o', label='Async MTE', capsize=5)
            plt.errorbar(sizes_base, means_sync, yerr=stds_sync, marker='s', label='Sync MTE', capsize=5)

            # Add labels and legend
            plt.xlabel('Size')
            plt.ylabel('Performance Ratio (MTE/Baseline)')
            plt.title(f'Performance Comparison for {work} (CPU {cpu})')
            plt.grid(True, linestyle='--', alpha=0.7)
            plt.legend()
            
            # Save the figure
            plt.tight_layout()
            plt.savefig(f'plots/cpu{cpu}_{work.replace(" ", "_")}.png')
            plt.close()
        else:
            sizes_async = sorted(data[-1][work].keys())
            means_sync = [sync_data[work][size][0][0] for size in sizes_async]
            stds_sync = [sync_data[work][size][0][1] for size in sizes_async]
            plt.figure(figsize=(10, 6))
            match = re.search(r'\((.*?)MTE\)', work)
            barrier = match.group(1).strip()
            plt.errorbar(sizes_async, means_sync, yerr=stds_sync, marker='s', label=barrier, capsize=5)

            # Add labels and legend
            plt.xlabel('Size')
            plt.ylabel('Performance Ratio (Sync MTE/Async MTE+Barrier)')
            plt.title(f'Performance Comparison for {work} (CPU {cpu})')
            plt.grid(True, linestyle='--', alpha=0.7)
            plt.legend()
            
            # Save the figure
            plt.tight_layout()
            plt.savefig(f'plots/cpu{cpu}_{work.replace(" ", "_")}.png')
            plt.close()

cpu_list = extract_cpus_from_bash_script("./run.sh")
for cpu in cpu_list:
    data, workload_set = read_file(cpu)
    plot(cpu, data, workload_set)