import argparse
import csv
from collections import defaultdict
import numpy as np
import matplotlib.pyplot as plt
import os

filename = "data.csv"
parser = argparse.ArgumentParser(description='parse')
parser.add_argument('-i', dest='input_path', help='file path')
args = parser.parse_args()
basename = os.path.basename(args.input_path)

# Store all measurements in a nested dict:
# results[core][operation][version][input_size] = list of times
results = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: defaultdict(list))))

with open(args.input_path, newline='') as csvfile:
    reader = csv.reader(csvfile)
    for row in reader:
        core = int(row[0])
        operation = row[1]      # sign / verify
        version = row[2]        # default / mtekmod / ...
        input_size = int(row[3]) / 8
        time_sec = float(row[4])

        results[core][operation][version][input_size].append(time_sec)

# Plot per core, separate figure for sign/verify
for core, operations in results.items():
    for operation, versions in operations.items():
        plt.figure(figsize=(8,5))
        plt.title(f"Core {core} - {operation}")
        plt.xlabel("Input Size")
        plt.ylabel("Relative Runtime")

        for version, measurements in versions.items():
            if version == "dr": continue
            # Aggregate by input size
            input_sizes = sorted(measurements.keys())
            y = []
            stds = []
            for size in input_sizes:
                times = np.array(measurements[size])
                baseline_times = np.array(versions["default"][size])
                print(f"{core},{operation},{version},{size}: {times}")
                yy = np.median(times) / np.median(baseline_times)
                y.append(yy)
                rel_stds = times / np.median(baseline_times)
                stds.append(rel_stds.std(ddof=1) if len(rel_stds) > 1 else 0)

            # Plot line + error bars
            # plt.bar(
            plt.errorbar(
                input_sizes, y, yerr=stds, 
                marker='o', capsize=4, label=version
            )

        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.xticks([64, 128, 256, 384, 512])
        # plt.yticks(np.arange(1, 3.5, 0.5))
        plt.xlim(60, 516)
        plt.legend(frameon=True, framealpha=1)
        # plt.show()
        plt.savefig(f"rsa_{operation}_{core}.{basename}.pdf", bbox_inches='tight')

