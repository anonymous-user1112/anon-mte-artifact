import argparse
import re
from collections import defaultdict
import os
import numpy as np
import matplotlib.pyplot as plt
from typing import List, Dict, Tuple
from matplotlib.ticker import MultipleLocator

### --- DATA PARSING --- ###

def parse_log_data(lines: List[str]) -> Dict[str, Dict[str, List[Tuple[int, float]]]]:
    """
    Parses log lines into a structured dictionary.

    Args:
        lines: A list of strings from the log file.

    Returns:
        A dictionary mapping binary names to their operational data.
        Example: {'bin_name': {'op_name': [(bufsize, value), ...]}}
    """
    # Regex to capture the data lines
    line_pattern = re.compile(
            r"ITER: \d+\s+bufsize (\d+): \([a-z0-9_]+(?:, [a-z0-9_]+)*\) = \((\d+(?:, \d+)+)\)$"
            )
    # Regex to find the operation names from the same line
    ops_pattern = re.compile(r"\(([^)]+)\)")

    # Use defaultdict for cleaner and more efficient data aggregation
    results = defaultdict(lambda: defaultdict(list))
    current_bin = "unknown_binary"

    for line in lines:
        line = line.strip()
        if not line:
            continue

        # A line ending in .txt is assumed to be the binary name for subsequent data
        if line.endswith(".txt"):
            current_bin = line
            continue

        match = line_pattern.match(line)
        if match:
            bufsize_str, nums_str = match.groups()
            bufsize = int(bufsize_str)

            # Extract operation names and numbers
            ops_match = ops_pattern.search(line)
            ops = ops_match.group(1).split(', ') if ops_match else []
            nums = [int(n) / 1_000_000 for n in nums_str.split(', ')]

            if len(ops) == len(nums):
                for op, num in zip(ops, nums):
                    results[current_bin][op].append((bufsize, num))
            else:
                print(f"Warning: Mismatch between ops and nums in line: {line}")

    return results

### --- PLOTTING --- ###

def create_plots(results: dict, output_prefix: str):
    """
    Generates and saves a line plot for each binary in the results.
    """
    # Dictionary to rename labels for the plot legend
    label_map = {'set_tags': 'dc gva'}
    # Dictionary to define a minimum x-axis value for certain operations
    x_axis_mins = {'stg': 16, 'st2g': 32, 'set_tags': 64}
    # Operations to skip during plotting
    ops_to_skip = {'memcpy'}

    for binary_name, data in results.items():
        if 'o0' in binary_name:
            continue
        if 'FLUSH' in binary_name:
            continue
        # Use the object-oriented approach (fig, ax) for better control
        fig, ax = plt.subplots(figsize=(6, 2.5))
        max_y_value = 0  # Track the max y-value for dynamic axis scaling

        for op, points in data.items():
            # if op in ops_to_skip:
            #     continue

            # --- Cleanly filter data and get plot-ready values ---
            min_x = x_axis_mins.get(op, 0) # Default to 0 if op not in map
            filtered_points = sorted([p for p in points if p[0] >= min_x])

            if not filtered_points:
                continue

            # Unzip the filtered points into x and y lists
            x_values, y_values = zip(*filtered_points)

            # --- Update max_y for automatic y-axis scaling ---
            max_y_value = max(max_y_value, max(y_values))

            # Get the display label, defaulting to the original op name
            label = label_map.get(op, op)
            print(label, y_values)
            ax.plot(x_values, y_values, label=label, marker='.', markersize=8)

        # --- Configure and style the plot ---
        ax.set_xscale('log', base=2)
        ax.set_xlabel("Buffer Size (bytes, log scale)")
        ax.set_ylabel("CPU Cycles (Millions)")
        ax.yaxis.set_major_locator(MultipleLocator(200))

        # ax.set_title(f"Performance of {binary_name}")
        ax.grid(True, which="both", linestyle='--', linewidth=0.5)
        ax.legend(ncols=2)

        # --- Automatically set y-axis limits with padding ---
        if max_y_value > 0:
            ax.set_ylim(bottom=0, top=max_y_value * 1.15) # 15% padding

        # Save the figure and close it to free up memory
        output_filename = f"{os.path.splitext(binary_name)[0]}.pdf"
        fig.savefig(output_filename, bbox_inches='tight')
        plt.close(fig)
        print(f"Saved plot to {output_filename}")

def main():
    """Main function to parse arguments and run the script."""
    parser = argparse.ArgumentParser(description="Parse performance logs and generate plots.")
    parser.add_argument('-i', dest='input_path', required=True, help='Path to the input log file.')
    args = parser.parse_args()

    try:
        with open(args.input_path, "r") as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"Error: The file '{args.input_path}' was not found.")
        return

    params = {
        "axes.labelsize": 16,
        "xtick.labelsize": 16,
        "ytick.labelsize": 16,
        "legend.fontsize": 16,
        "figure.titlesize": 8,
        "text.usetex": True,
        "font.family": "serif",
        "font.weight": "semibold",
        "savefig.dpi": 1000,
        "figure.dpi": 1000,
        "text.latex.preamble": r"""
            %\usepackage[libertine]{newtxmath}
            %\usepackage{libertine}
            %\usepackage{zi4}
            \usepackage{newtxtext} % usenix
            \usepackage{newtxmath}
            """,
    }
    plt.rcParams.update(params)

    parsed_data = parse_log_data(lines)

    output_prefix = os.path.splitext(args.input_path)[0]
    create_plots(parsed_data, output_prefix)


if __name__ == "__main__":
    main()
