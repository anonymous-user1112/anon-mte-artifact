import argparse
from logging import log
import matplotlib.pyplot as plt
import re
from collections import defaultdict
import os
import numpy as np
import pprint
from itertools import compress

# Function to compute 90% confidence interval average
def confidence_interval_mean(data, confidence=90):
    lower_percentile = (100 - confidence) / 2
    upper_percentile = 100 - lower_percentile
    filtered_data = np.percentile(data, [lower_percentile, upper_percentile])
    mask = (data >= filtered_data[0]) & (data <= filtered_data[1])
    return np.mean(data[mask])

def get_list_and_append(d, key, value):
    d[key] = d.get(key, [])
    d[key].append(value)

def tex_monospace(string):
    return r"\texttt{" + string + "}"

# Function to parse the data and calculate the ratio
def parse_and_plot(data, filename):
    # Regex pattern to match the header and subsequent digit lines
    line_pattern = re.compile(r"ITER: (\d+)\s+bufsize (\d+): \(([a-z0-9_]+(, [a-z0-9_]+)+)\) = \((\d+(, \d+)+)\)$")

    # Data structure to store parsed results
    results = {}
    # results[binary_name][xaxis_bufsize][op_name] = result

    lines = data

    bin = "Placeholder"
    for line in lines:
        line_match = line_pattern.match(line)
        if line_match is None:
            if line.rstrip().endswith("txt"):
                bin = line.rstrip()
            continue
        i, bufsize, ops, _, nums, _ = line_match.groups()
        # print(i, bufsize)
        ops = ops.split(', ')
        nums = nums.split(', ')
        # print(ops)
        # print(nums)
        for i in range(len(ops)):
            per_bin = results.get(bin, {})
            per_ops = per_bin.get(ops[i], [])
            per_ops.append([int(bufsize), int(nums[i]) / 1_000_000])
            per_bin[ops[i]] = per_ops
            results[bin] = per_bin

    pprint.pprint(results)

    x_axis_mins = { 'stg': 16, 'st2g': 32, 'set_tags': 64, 'scudo': 16, 'glibc': 16, 'memcpy': 16 }

    for bin, result in results.items():
        print(bin)
        plt.figure(figsize=(6, 2.5))
        plt.xscale('log', base=2)
        for op, nums in result.items():
            label = op
            # if label == 'memcpy':
            #     continue
            if label == 'set_tags':
                label = 'dc gva'
            valid_indices = [v[0] >= x_axis_mins[op] for v in nums]
            print(valid_indices)
            x_values = [v[0] for v in nums]
            y_values = [v[1] for v in nums]
            valid_x = list(compress(x_values, valid_indices))
            valid_y = list(compress(y_values, valid_indices))
            print(valid_x, valid_y)
            # label = tex_monospace(label)
            plt.plot(valid_x, valid_y, label=label)

        plt.xlabel("Buffer Size in bytes (log scale)")
        plt.ylabel("CPU Cycles (millions)")
        plt.yticks(np.arange(0, 100, 10))
        plt.legend(ncols=2, frameon=True, framealpha=1)
        plt.rc('axes', labelsize=16)    # fontsize of the x and y labels
        plt.rc('xtick', labelsize=16)    # fontsize of the tick labels
        plt.rc('ytick', labelsize=12)    # fontsize of the tick labels
        plt.rc('legend', fontsize=14)    # legend fontsize
        plt.rc('figure', titlesize=8)  # fontsize of the figure title
        # plt.grid()
        # plt.show()
        plt.savefig(f"{bin}.pdf", bbox_inches='tight')


def main():
    parser = argparse.ArgumentParser(description='parse')
    parser.add_argument('-i', dest='input_path', help='file path')
    args = parser.parse_args()
    with open(args.input_path, "r") as f:
        lines = f.readlines()
        parse_and_plot(lines, os.path.splitext(args.input_path)[0])


if __name__ == "__main__":
    plt.style.use(style='seaborn-v0_8-whitegrid')
    #plt.rcParams['font.serif'] = ['Bitstream Charter']
    #plt.rc('text.latex', preamble=r"\usepackage[T1]{fontenc}%\usepackage[tt=false]{libertine}%\usepackage[libertine]{newtx}%\setmonofont[StylisticSet=3]{inconsolata}")

    params = {
            'text.usetex': True,
            'font.family': 'serif',
            'font.weight': 'semibold',
            # 'axes.titlesize': 12,
            # 'axes.labelsize': 10,
            # 'xtick.labelsize': 10,
            # 'ytick.labelsize': 10,
            'savefig.dpi': 1000,
            'figure.dpi': 1000,
            'text.latex.preamble':
            r"""
            \usepackage[libertine]{newtxmath}
            \usepackage{libertine}
            \usepackage{zi4}
            """,
            }
    plt.rcParams.update(params)

    main()

