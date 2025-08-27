import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.colors import LinearSegmentedColormap
from pathlib import Path
from typing import Dict, List
import shutil
from matplotlib import gridspec
# A type alias for clarity
RawData = Dict[int, Dict[int, List[int]]]

def _read_file(filepath: Path, cold: int, step: int) -> RawData | None:
    if not filepath.exists():
        print(f"Warning: Input file not found: {filepath}")
        return None

    data = {}
    curr_keys = []
    with open(filepath) as f:
        for line in f:
            try:
                keys = list(map(int, line.strip().split(", ")))
                if len(keys) == 5:
                    curr_keys = keys
                elif len(keys) == 1:
                    is_target_data = (
                            curr_keys[0] == step
                            and curr_keys[3] == cold
                            and curr_keys[1] >= 1
                            and curr_keys[2] >= 1
                            )
                    if is_target_data:
                        # Use setdefault for cleaner nested dictionary creation
                        level1 = data.setdefault(curr_keys[1], {})
                        level2 = level1.setdefault(curr_keys[2], [])
                        level2.append(keys[0])
            except (ValueError, IndexError):
                print(f"Warning: Skipping malformed line in {filepath}: {line.strip()}")
    return data

def _parse_measurements(array1: List[int], array2: List[int]) -> tuple[float, float]:
    ratios = np.array(array1) / np.array(array2)
    trimmed_ratios = np.sort(ratios)[2:-2]
    return np.mean(trimmed_ratios), np.std(trimmed_ratios)

class _AdaptiveLowEndNormalize(mcolors.Normalize):
    def __init__(self, data_max, vmin=1.0, vmax=4.0):
        super().__init__(vmin=vmin, vmax=vmax, clip=True)
        used_frac = max(1e-6, (data_max - vmin) / (vmax - vmin))
        self.gamma = 0.35 + 0.65 * used_frac

    def __call__(self, value, clip=None):
        normed = super().__call__(value, clip=clip)
        with np.errstate(invalid="ignore"):
            return np.power(normed, self.gamma)




def _plot_heatmap(mte_async_data1: RawData, mte_async_data2: RawData, base_data1: RawData, base_data2: RawData, cold: int, step1: int, step2: int, output_dir: Path):
    x1, y1, means1, stds1 = [], [], [], []
    
    for key1 in base_data1:
        for key2 in base_data1[key1]:
            if key1 in mte_async_data1 and key2 in base_data1[key1]:
                mean, std = _parse_measurements(mte_async_data1[key1][key2], base_data1[key1][key2])
                if key1 < 256 or key2 < 256:
                    continue
                x1.append(key1)
                y1.append(key2)
                means1.append(mean)
                stds1.append(std)

    x2, y2, means2, stds2 = [], [], [], []

    for key1 in base_data2:
        for key2 in base_data2[key1]:
            if key1 in mte_async_data2 and key2 in base_data2[key1]:
                mean, std = _parse_measurements(mte_async_data2[key1][key2], base_data2[key1][key2])
                if key1 < 256 or key2 < 256:
                    continue
                x2.append(key1)
                y2.append(key2)
                means2.append(mean)
                stds2.append(std)

    if not means1:
        print(f"Warning: No overlapping data found for cold={cold}, step={step1}. Skipping plot.")
        return

    if not means2:
        print(f"Warning: No overlapping data found for cold={cold}, step={step2}. Skipping plot.")
        return

    print(means1)
    print(means2)


    df1 = pd.DataFrame({"x": x1, "y": y1, "data": means1})
    heatmap_data1 = df1.pivot(index="y", columns="x", values="data")
    data_max1 = float(np.nanmax(heatmap_data1.values))

    df2 = pd.DataFrame({"x": x2, "y": y2, "data": means2})
    heatmap_data2 = df2.pivot(index="y", columns="x", values="data")
    data_max2 = float(np.nanmax(heatmap_data2.values))

    fig = plt.figure(figsize=(10, 4))
    gs = gridspec.GridSpec(
        1, 3, figure=fig,
        width_ratios=[1, 1, 0.05],  # last thin axis for colorbar
        wspace=0.15
    )
    ax0 = fig.add_subplot(gs[0, 0])
    ax1 = fig.add_subplot(gs[0, 1], sharey=ax0)
    cax = fig.add_subplot(gs[0, 2])  # colorbar axis
    # fig, axes = plt.subplots(1, 2, figsize=(10, 4), sharey=True)

    cmap = LinearSegmentedColormap.from_list(
        "b_y_r",
        [
            (0.0, "#1f3b99"),  # deep blue at 1
            (1 / 3, "#ffd500"),  # yellow at 2
            (1.0, "#c00000"),  # red at 4
        ],
    )

    norm = mcolors.Normalize(vmin=1.0, vmax=4.0)

    print(heatmap_data1)
    print(heatmap_data2)
    # norm = _AdaptiveLowEndNormalize(data_max=data_max, vmin=1.0, vmax=4.0)
    h1 = sns.heatmap(
            heatmap_data1, 
            ax=ax0, 
            cmap=cmap, 
            norm=norm, linewidths=0.5,
            cbar=False
        )
    ax0.set_title(f"Stride ($S$)={step1}")
    ax0.set_ylabel(r"Linked list Length ($L$)")
    ax0.set_xlabel(r"Array size ($A$)")
    ax0.invert_yaxis()

    h2 = sns.heatmap(
            heatmap_data2, 
            ax=ax1, 
            cmap=cmap, 
            norm=norm, linewidths=0.5,
            cbar=True,
            cbar_ax=cax
        )
    ax1.set_title(f"Stride ($S$)={step2}")
    ax1.set_ylabel("")  # shared
    ax1.set_xlabel(r"Array size ($A$)")
    ax1.invert_yaxis()
    ax1.tick_params(axis='y', which='both', left=False, labelleft=False)

    cbar = h2.collections[0].colorbar
    cbar.set_label("Async/Base", rotation=90)
    cbar.ax.tick_params(length=3)
    # # Single colorbar spanning both
    # cbar = fig.colorbar(
    #     h1.collections[0],
    #     ax=axes,
    #     fraction=0.046,
    #     pad=0.04
    # )
    # cbar.set_label("Async/Base")

    # fig.suptitle(f"cold={cold} (max ratio across both: {combined_max:.3f})", fontsize=14, y=1.02)

    fig.tight_layout()

    output_path = output_dir / f"cold-{cold}-step1-{step1}-step2-{step2}.jpg"
    fig.savefig(output_path, format="jpg", bbox_inches="tight")
    plt.close()
    # print(f"Saved heatmap to {output_path}, max {df['data'].max()}, min {df['data'].min()}")

def generate_heatmaps(
        input_dir_cold,
        input_dir_warm,
        output_dir,
        steps_to_plot,
        colds_to_plot = [0, 1]
        ):
    # Convert string paths to Path objects for robust handling
    dir_cold, dir_warm, dir_out = Path(input_dir_cold), Path(input_dir_warm), Path(output_dir)
    dir_out.mkdir(parents=True, exist_ok=True)

    params = {
        "axes.labelsize": 16,
        "axes.titlesize": 20,
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

    all_data = {}
    for cold_val in colds_to_plot:
        for step_val in steps_to_plot:
            input_dir = dir_cold if cold_val == 0 else dir_warm
            prefix = "cold" if cold_val == 0 else "warm"

            async_file = input_dir / f"{prefix}_async.txt"
            baseline_file = input_dir / f"{prefix}_base.txt"

            async_data = _read_file(async_file, cold_val, step_val)
            baseline_data = _read_file(baseline_file, cold_val, step_val)
            all_data[(cold_val, step_val)] = (async_data, baseline_data)
    
    _plot_heatmap(all_data[(1, 4)][0], all_data[(1, 128)][0], all_data[(1, 4)][1], all_data[(1, 128)][1], 1, 4, 128, dir_out)

def extract_steps_from_bash_script(script_path):
    with open(script_path, "r") as file:
        for line in file:
            if "declare -a steps=" in line:
                # Extract the content between parentheses
                content = line.split("(")[1].split(")")[0]
                # Split by space and convert to integers
                steps = [int(num) for num in content.split()]
                return steps
    return []

if __name__ == "__main__":
    STEPS = extract_steps_from_bash_script("./run.sh")
    print(STEPS)
    print("--- Starting heatmap generation ---")
    # generate_heatmaps(
    #         input_dir_cold="../../mte-root/benchmarks/heatmap-ampere1-cpu1",
    #         input_dir_warm="../../mte-root/benchmarks/heatmap-ampere1-cpu1",
    #         output_dir="../../mte-root/benchmarks/heatmap-ampere1-cpu1",
    #         steps_to_plot=STEPS,
    #         colds_to_plot=[1], # we only have warm
    #         )
    generate_heatmaps(
            input_dir_cold="../../mte-root/benchmarks/heatmap-p8-cpu4",
            input_dir_warm="../../mte-root/benchmarks/heatmap-p8-cpu4",
            output_dir="../../mte-root/benchmarks/heatmap-p8-cpu4",
            steps_to_plot=STEPS,
            colds_to_plot=[0, 1],
            )
    # generate_heatmaps(
    #         input_dir_cold="../../mte-root/benchmarks/heatmap-p9-cpu4",
    #         input_dir_warm="../../mte-root/benchmarks/heatmap-p9-cpu4",
    #         output_dir="../../mte-root/benchmarks/heatmap-p9-cpu4",
    #         steps_to_plot=STEPS,
    #         colds_to_plot=[0, 1],
    #         )
    print("--- Heatmap generation complete ---")
