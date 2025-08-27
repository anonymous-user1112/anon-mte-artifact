import csv
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Any, Optional, Set

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.ticker import MultipleLocator

COLOR_CYCLE = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b']

def _parse_data(filepath):
    """
    (Internal) Parses the input CSV file into a nested dictionary.
    """
    results = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: defaultdict(list))))
    try:
        with open(filepath, newline='') as csvfile:
            reader = csv.reader(csvfile)
            for row in reader:
                try:
                    core, op, ver = int(row[0]), row[1], row[2]
                    size, time = int(row[3]) / 8, float(row[4])
                    results[core][op][ver][size].append(time)
                except (ValueError, IndexError):
                    print(f"Warning: Skipping malformed row: {row}")
    except FileNotFoundError:
        print(f"Error: Input file not found at '{filepath}'")
        return {}
    return results

def _create_plot(core, operation, versions_data, output_dir, break_config):
    """
    (Internal) Creates and saves a single plot for a given core and operation.
    """
    use_broken_axis = break_config is not None

    if use_broken_axis:
        fig, (ax_top, ax_bottom) = plt.subplots(
                2, 1, sharex=True, figsize=(4, 5), gridspec_kw={'height_ratios': [1.5, 2]}
                )
        axes = [ax_top, ax_bottom]
        fig.subplots_adjust(hspace=0.1)
    else:
        fig, ax = plt.subplots(figsize=(8, 5))
        axes = [ax]

    main_ax = axes[-1]
    for ax in axes:
        ax.set_prop_cycle(color=COLOR_CYCLE)

    if "default" not in versions_data:
        print(f"Warning: 'default' version not found for core {core}, op '{operation}'. Cannot plot.")
        plt.close(fig)
        return

    for version, measurements in sorted(versions_data.items()):
        if version == "dr" and not use_broken_axis:
            continue

        input_sizes = sorted(measurements.keys())
        y_values, stds = [], []

        for size in input_sizes:
            times = np.array(measurements[size])
            baseline_times = np.array(versions_data["default"][size])
            median_baseline = np.median(baseline_times)

            yy = np.nan if median_baseline == 0 else np.median(times) / median_baseline
            y_values.append(yy)

            rel_stds = times / median_baseline if median_baseline != 0 else times
            stds.append(rel_stds.std(ddof=1) if len(rel_stds) > 1 else 0)

        for ax in axes:
            ax.errorbar(input_sizes, y_values, yerr=stds, marker='o', capsize=4, label=version)

    main_ax.set_xlabel("Input Size (bytes)")
    fig.text(0.01, 0.5, "Relative Runtime", va='center', rotation='vertical', size=16)
    main_ax.set_xticks([64, 128, 256, 384, 512])
    main_ax.set_xmargin(0.02)

    if use_broken_axis:
        ax_top.set_ylim(bottom=break_config['high'])
        ax_bottom.set_ylim(top=break_config['low'])
        ax_top.spines['bottom'].set_visible(False)
        ax_bottom.spines['top'].set_visible(False)
        ax_top.tick_params(axis='x', which='both', bottom=False)
        for ax in axes: ax.grid(True, linestyle='--', alpha=0.7)
        else:
            main_ax.grid(True)
        main_ax.set_ylim(bottom=0.9)
        ax_top.yaxis.set_major_locator(MultipleLocator(break_config['top_stride']))
        ax_bottom.yaxis.set_major_locator(MultipleLocator(break_config['bottom_stride']))

    handles, labels = main_ax.get_legend_handles_labels()
    fig.legend(handles, labels, bbox_to_anchor=(0.975, 0.565), ncol=1, framealpha=1)
    fig.tight_layout(rect=[0.03, 0, 1, 0.9])

    if use_broken_axis:
        fig.subplots_adjust(wspace=0.05, hspace=0.05)
        pos_bottom, pos_top = ax_bottom.get_position(), ax_top.get_position()
        rect = patches.Rectangle(
                (pos_bottom.x0, pos_bottom.y1), width=pos_bottom.width,
                height=pos_top.y0 - pos_bottom.y1, transform=fig.transFigure,
                facecolor='#808080', edgecolor='none', clip_on=False, zorder=0
                )
        fig.add_artist(rect)

    output_filename = output_dir / f"rsa_{operation}_{core}.pdf"
    plt.savefig(output_filename, bbox_inches='tight')
    plt.close(fig)
    print(f"Saved plot to '{output_filename}'")

def generate_plots_from_csv(
        input_path,
        output_dir,
        operations_to_plot=None,
        break_configs=None
        ):
    # Ensure arguments are in the correct format
    input_path = Path(input_path)
    output_dir = Path(output_dir)
    if operations_to_plot is None:
        operations_to_plot = {'sign', 'verify'} # Default to all known operations
    if break_configs is None:
        break_configs = {}

    output_dir.mkdir(parents=True, exist_ok=True)
    raw_data = _parse_data(input_path) # RawData = Dict[int, Dict[str, Dict[str, Dict[float, List[float]]]]]

    if not raw_data:
        return

    # Set LaTeX font parameters for plotting
    params = {
        "axes.labelsize": 16,
        "xtick.labelsize": 16,
        "ytick.labelsize": 16,
        "legend.fontsize": 13,
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

    for core, operations in raw_data.items():
        for operation, versions in operations.items():
            if operation in operations_to_plot:
                # Get the specific break config for this operation, or None if not defined
                current_break_config = break_configs.get(operation)
                _create_plot(core, operation, versions, output_dir, current_break_config)

if __name__ == "__main__":
    ops_to_generate = {'sign', 'verify'}

    # --- Run the plotting function ---
    print("--- Running plotting task ---")
    generate_plots_from_csv(
            input_path="data_a1.csv",
            output_dir="memtrace-a1",
            operations_to_plot=ops_to_generate,
            break_configs={
                'sign': {'low': 2.0, 'high': 60, 'top_stride': 25, 'bottom_stride': 0.25},
                'verify': {'low': 13, 'high': 50, 'top_stride': 25, 'bottom_stride': 2.5}
                }
            )

    generate_plots_from_csv(
            input_path="data_p8_2.csv",
            output_dir="memtrace-p8",
            operations_to_plot=ops_to_generate,
            break_configs={
                'sign': {'low': 3.5, 'high': 30, 'top_stride': 25, 'bottom_stride': 0.25},
                'verify': {'low': 26, 'high': 30, 'top_stride': 25, 'bottom_stride': 2.5}
                }
            )
    print("--- Plotting task complete ---")
