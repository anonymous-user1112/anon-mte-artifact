import argparse
import statistics
from logging import log
import matplotlib.pyplot as plt
import re
import csv
from collections import defaultdict
import os
import numpy as np
import pprint
from itertools import compress

core_names = { "little": "Little Core", "big": "Big Core", "cxc": "Perf Core", "1": "AmpereOne"}
# https://colorbrewer2.org/#type=diverging&scheme=PRGn&n=6
# https://colorbrewer2.org/#type=diverging&scheme=RdBu&n=7
# colors = ['#2166ac']
# colors = ['#b2182b','#ef8a62','#fddbc7','#f7f7f7','#d1e5f0','#67a9cf','#2166ac']
one_per_core = [ '#bcbddc', '#a1d99b', '#fdae6b']
ampere = ['#2171b5', '#6baed6', '#bdd7e7', '#eff3ff']
colors=one_per_core

def get_summary_between_samples(l1 : list, l2 : list):
    A = np.array(l1)
    B = np.array(l2)
    ratios = A[:, None] / B[None, :]  # shape (len(A), len(B))
    norm_min = np.min(ratios)
    norm_max = np.max(ratios)
    norm_median = np.median(ratios)
    return norm_median, norm_min, norm_max

def nested_dict():
    return [[], []]

def parse_and_plot(infile, outfile):
    basename = os.path.basename(infile)
    results = csv_parse(infile)

    fig, ax = plt.subplots(figsize=(12, 3))

    # Define M and N
    anykey = list(results.keys())[0]
    progs = list(results[anykey].keys())
    progs.append("geomean")
    M = len(progs)
    cores_reversed = list(reversed(results.keys()))
    N = len(cores_reversed)

    # Calculate bar width and base positions
    x = np.arange(M)
    total_group_width = 0.84
    bar_width = total_group_width / N

    max_y_value = 0

    for i, core in enumerate(cores_reversed):
        benches = results[core]
        y = []
        yminerr = []
        ymaxerr = []
        for name, numbers in benches.items():
            med, min_val, max_val = get_summary_between_samples(numbers[1], numbers[0])
            y.append(med)
            yminerr.append(med - min_val)
            ymaxerr.append(max_val - med)

        if y:
            y.append(statistics.geometric_mean(y))
        else:
            y.append(0)
        yminerr.append(0)
        ymaxerr.append(0)
        yerr = [yminerr, ymaxerr]

        current_max = np.max(np.array(y) + np.array(ymaxerr))
        if current_max > max_y_value:
            max_y_value = current_max

        offset = (i - (N - 1) / 2) * bar_width
        positions = x + offset
        ax.bar(positions, y, yerr=yerr, label=core_names[core], width=bar_width, edgecolor='black', color=colors[i])

    # Set x-ticks and labels
    ax.set_xticks(x)
    ax.set_xticklabels(progs, rotation=60, ha="right", rotation_mode='anchor')
    ax.set_yticks(np.arange(0, max_y_value * 1.1, 0.5))
    ax.set_xmargin(0.01)
    ax.axhline(y=1.0, color="black", linestyle="--")

    # Styling
    ax.tick_params(axis='x', direction='out', length=2)
    ax.set_ylabel("Normalized Runtime")
    ax.legend(ncols=1, frameon=True, framealpha=0.8)
    plt.rc('axes', labelsize=16)
    plt.rc('xtick', labelsize=16)
    plt.rc('ytick', labelsize=16)
    plt.rc('legend', fontsize=16)

    ax.set_xmargin(0.01)
    ax.minorticks_on()
    ax.grid(axis='y', linestyle='--', alpha=0.7) # Added a grid for readability

    fig.savefig(f"{outfile}", bbox_inches='tight')
    plt.close(fig)


def csv_parse(filename):
    results = defaultdict(lambda: defaultdict(nested_dict))
    with open(filename, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            # Example: access values
            core = row['core']
            name = row['name']
            baseline = float(row['baseline'])
            mte = float(row['mte'])
            # print(f"Core: {core}, Name: {name}, Baseline: {baseline}, MTE: {mte}")
            results[core][name][0].append(baseline)
            results[core][name][1].append(mte)

    return results


def main():
    parser = argparse.ArgumentParser(description='parse')
    parser.add_argument('-i', dest='input_path', help='file path')
    parser.add_argument('-o', dest='out', help='<output>.pdf')
    args = parser.parse_args()
    parse_and_plot(args.input_path, args.out)


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
            %\usepackage[libertine]{newtxmath}
            %\usepackage{libertine}
            %\usepackage{zi4}
            \usepackage{newtxtext} % usenix
            \usepackage{newtxmath}
            """,
            }
    plt.rcParams.update(params)
    main()

