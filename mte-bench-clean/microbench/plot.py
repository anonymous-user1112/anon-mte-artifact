import argparse
import os
import re
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Set, Tuple, Any

import matplotlib.pyplot as plt
import numpy as np

# A type alias for our deeply nested data structure for better readability
RawData = Dict[int, Dict[str, Dict[int, List[int]]]]

def wrap_to_signed(value):
    return np.int32(np.uint32(value))

def slugify(value: str) -> str:
    """
    Normalizes a string by removing or replacing characters that are not
    filesystem-friendly.
    """
    value = re.sub(r"[^\w\s-]", "", value).strip().lower()
    value = re.sub(r"[-\s]+", "_", value)
    return value


def read_data_file(input_dir: Path, cpu: int) -> Tuple[RawData | None, Set[str] | None]:
    """
    Reads and parses a single data file for a given CPU.

    Args:
        input_dir: The directory containing the result files.
        cpu: The CPU number to process.

    Returns:
        A tuple containing the parsed data and a set of workload names,
        or (None, None) if the file is not found.
    """
    file_path = input_dir / f"micro_{cpu}.txt"
    if not file_path.exists():
        print(f"Warning: File {file_path} not found. Skipping CPU {cpu}.")
        return None, None

    # Structure: data[mte_mode][workload_name][size] = [run_1, run_2, ...]
    data = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))
    workload_set = set()
    current_size = 0
    current_mte_mode = -2  # Default value indicating no mode set yet

    with open(file_path) as f:
        for line in f:
            line = line.strip()
            keys = line.split(",")

            # Check for header line (e.g., "-1,64,0,0")
            if len(keys) == 4:
                try:
                    current_mte_mode = wrap_to_signed(keys[0])
                    current_size = wrap_to_signed(keys[1])
                except (ValueError, IndexError):
                    print(f"Warning: Could not parse header line: {line}")
            # Check for data line
            elif len(line) > 20:
                line = line.replace(" (MTE)", "")
                parts = line.split(" ")
                try:
                    # The value is the second to last element
                    number = int(parts[-2])
                    # The workload name is everything before that
                    name = " ".join(parts[:-2])
                    data[current_mte_mode][name][current_size].append(number)
                    workload_set.add(name)
                except (ValueError, IndexError):
                    print(f"Warning: Could not parse data line: {line}")
    return data, workload_set


def process_ratios(data: RawData, workload_set: Set[str]) -> Dict[str, Dict[str, Any]]:
    """
    Calculates performance ratios and statistics from the raw data.

    This function separates the data processing logic from plotting.

    Returns:
        A dictionary containing all necessary information for plotting each workload.
    """
    plot_tasks = {}

    for work in workload_set:
        task = {"workload_name": work, "series": {}}

        # Case 1: Workload has a baseline (mte_mode=0) for direct comparison
        if work in data.get(0, {}):
            sizes = sorted(data[0][work].keys())
            means_async, stds_async = [], []
            means_sync, stds_sync = [], []

            for size in sizes:
                base_vals = np.array(data[0][work][size], dtype=float)
                async_vals = np.array(
                    data.get(-1, {}).get(work, {}).get(size, []), dtype=float
                )
                sync_vals = np.array(
                    data.get(1, {}).get(work, {}).get(size, []), dtype=float
                )

                # Calculate ratios safely, avoiding division by zero
                with np.errstate(divide="ignore", invalid="ignore"):
                    print(async_vals)
                    async_ratio = np.nan_to_num(async_vals / base_vals, nan=1.0)
                    print(sync_vals)
                    sync_ratio = np.nan_to_num(sync_vals / base_vals, nan=1.0)

                means_async.append(np.mean(async_ratio))
                stds_async.append(np.std(async_ratio))
                means_sync.append(np.mean(sync_ratio))
                stds_sync.append(np.std(sync_ratio))

            task["sizes"] = sizes
            task["series"]["Async MTE"] = (means_async, stds_async)
            task["series"]["Sync MTE"] = (means_sync, stds_sync)
            task["ylabel"] = "Performance Ratio (MTE/Baseline)"

        # Case 2: Workload is a barrier test, compared against a related workload
        else:
            work_sync_equiv = work.replace(" (DSB MTE)", "").replace(" (DMB MTE)", "")

            # Ensure the corresponding sync data exists
            if work_sync_equiv not in data.get(1, {}):
                continue

            sizes = sorted(data.get(-1, {}).get(work, {}).keys())
            means_sync, stds_sync = [], []

            for size in sizes:
                async_barrier_vals = np.array(data[-1][work][size], dtype=float)
                sync_vals = np.array(data[1][work_sync_equiv][size], dtype=float)

                with np.errstate(divide="ignore", invalid="ignore"):
                    ratio = np.nan_to_num(sync_vals / async_barrier_vals, nan=1.0)

                means_sync.append(np.mean(ratio))
                stds_sync.append(np.std(ratio))

            # Extract barrier type from the name for the legend
            match = re.search(r"\((.*?)MTE\)", work)
            barrier_label = match.group(1).strip() if match else "Sync vs Async+Barrier"

            task["sizes"] = sizes
            task["series"][barrier_label] = (means_sync, stds_sync)
            task["ylabel"] = "Perf. Ratio (Sync / Async+Barrier)"

        if task["series"]:  # Only add task if it has data
            plot_tasks[work] = task

    return plot_tasks


def generate_plots(plot_tasks: Dict[str, Dict[str, Any]], cpu: int, output_dir: Path):
    """
    Generates and saves plots based on the processed data.

    Args:
        plot_tasks: A dictionary where each item contains all info for one plot.
        cpu: The current CPU number, used for titling and filenames.
        output_dir: The directory where plots will be saved.
    """
    for work, task in plot_tasks.items():
        fig, ax = plt.subplots(figsize=(10, 6))
        max_y = 0

        for label, (means, stds) in task["series"].items():
            sizes = task["sizes"]
            ax.errorbar(
                sizes,
                means,
                yerr=stds,
                marker="o",
                label=label,
                capsize=5,
                linestyle="-",
            )
            # Update max_y for automatic axis scaling, considering error bars
            max_y = max(max_y, np.max(np.array(means) + np.array(stds)))

        ax.set_xlabel("Size")
        ax.set_ylabel(task["ylabel"])
        ax.set_title(f"Performance for {work} (CPU {cpu})")
        ax.grid(True, linestyle="--", alpha=0.7)
        ax.legend()

        # Set y-axis limit with a 10% padding
        if max_y > 0:
            ax.set_ylim(bottom=0, top=max_y * 1.1)

        # Save the figure with a filesystem-safe name
        safe_filename = slugify(work)
        output_path = output_dir / f"cpu{cpu}_{safe_filename}.pdf"
        fig.savefig(output_path, bbox_inches="tight")
        plt.close(fig)
    print(f"Generated {len(plot_tasks)} plots for CPU {cpu} in '{output_dir}/'")


def main():
    """Main entry point for the script."""
    parser = argparse.ArgumentParser(
        description="Parse mte microbenchmark results and plot them.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--cpus",
        required=True,
        nargs="+",
        type=int,
        help="List of CPU numbers to process (e.g., --cpus 0 1 4).",
    )
    parser.add_argument(
        "--input-dir",
        type=Path,
        default=Path("result_micro_p8"),
        help="Directory containing the input 'micro_cpu.txt' files.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("plots"),
        help="Directory where output plots will be saved.",
    )
    args = parser.parse_args()

    # Create the output directory if it doesn't exist
    args.output_dir.mkdir(parents=True, exist_ok=True)

    for cpu in args.cpus:
        raw_data, workload_set = read_data_file(args.input_dir, cpu)
        if raw_data:
            plot_tasks = process_ratios(raw_data, workload_set)
            generate_plots(plot_tasks, cpu, args.output_dir)


if __name__ == "__main__":
    main()
