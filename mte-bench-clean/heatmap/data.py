import argparse
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.colors import LinearSegmentedColormap

def read_file(fn, cold, step):
    data = {}
    curr_keys = []
    with open(fn) as f:
        for line in f:
            keys = list(map(int, line.strip().split(", ")))
            if len(keys) == 5:
                curr_keys = keys
            if len(keys) == 1:
                if(curr_keys[0]==step and curr_keys[3]==cold and curr_keys[1]>=1 and curr_keys[2]>=1):
                    data.setdefault(curr_keys[1], {})
                    data[curr_keys[1]].setdefault(curr_keys[2], [])
                    data[curr_keys[1]][curr_keys[2]].append(int(keys[0])) # level 1 -> level 2 -> data
            
    f.close()
    return data


def parse(raw_array1, raw_array2):
    array1 = np.array(raw_array1)
    array2 = np.array(raw_array2)
    # Element-wise division
    ratios = array1 / array2
    sorted_ratios = np.sort(ratios)
    # Trim top 2 and bottom 2 values
    trimmed_ratios = sorted_ratios[2:-2]
    # Compute mean and standard deviation
    mean_ratio = np.mean(trimmed_ratios)
    std_ratio = np.std(trimmed_ratios)
    return(mean_ratio, std_ratio)

class AdaptiveLowEndNormalize(mcolors.Normalize):
    """
    Keep global vmin=1, vmax=4 for comparability, but stretch the occupied
    lower band so small spans (e.g. up to 1.2) still use most of the colormap.
    We raise the normalized value to a power < 1 (gamma) that depends on how
    much of the range is actually used.
    """
    def __init__(self, data_max, vmin=1.0, vmax=4.0):
        super().__init__(vmin=vmin, vmax=vmax, clip=True)
        # Fraction of the full (1..4) range actually used
        used_frac = max(1e-6, (data_max - vmin) / (vmax - vmin))
        # Map used_frac -> gamma in [0.35, 1.0] (smaller span => stronger stretch)
        self.gamma = 0.35 + 0.65 * used_frac

    def __call__(self, value, clip=None):
        normed = super().__call__(value, clip=clip)
        # Power (gamma) where gamma<1 expands low differences
        with np.errstate(invalid='ignore'):
            return np.power(normed, self.gamma)


def plot(mte_async_data, base_data, cold, step):
    x=[]
    y=[]
    means_async_base=[]
    stds_async_base=[]

    for key1 in base_data:
        for key2 in base_data[key1]:
            x.append(key1)
            y.append(key2)
            mean, std = parse(mte_async_data[key1][key2], base_data[key1][key2])
            means_async_base.append(mean)
            stds_async_base.append(std)
            # print(str(key1)+" "+str(key2)+" "+str(mean)+" "+str(np.mean(mte_sync_data[key1][key2]))+" "+str(np.mean(base_data[key1][key2])))
    
    #heatmap
    df = pd.DataFrame({'x': x, 'y': y, 'data': means_async_base})
    print(max(means_async_base))
    # Pivot to create a grid for heatmap
    heatmap_data = df.pivot(index='y', columns='x', values='data')
    data_max = float(np.nanmax(heatmap_data.values))
    data_min = float(np.nanmin(heatmap_data.values))

    plt.figure(figsize=(6, 4))
    # cmap = sns.color_palette("tab20", n_colors=10)

    # Custom colormap:
    #  value 1 -> blue, value 2 -> yellow, value 4 -> red
    # Normalize positions to 0..1 over fixed range [1,4]:
    #  pos(1)=0, pos(2)=(2-1)/3=1/3, pos(4)=(4-1)/3=1
    blue_to_yellow_to_red = LinearSegmentedColormap.from_list(
        "b_y_r",
        [
            (0.0,  "#1f3b99"),   # deep blue at 1
            (1/3,  "#ffd500"),   # yellow at 2
            (1.0,  "#c00000"),   # red at 4
        ]
    )

    adaptive_norm = AdaptiveLowEndNormalize(data_max=data_max, vmin=1.0, vmax=4.0)
    ax = sns.heatmap(heatmap_data, 
        annot=False, 
        cmap=blue_to_yellow_to_red, 
        norm=adaptive_norm,
        linewidths=0.5,
        cbar_kws={"label": f"Async/Base (γ={adaptive_norm.gamma:.2f})"}
    )
    ax.invert_yaxis()

    plt.xlabel("Bytes per Record")
    plt.ylabel("Linked list Length")

    plt.savefig("./plots/cold-%d-step-%d.jpg" % (cold, step), format="jpg", bbox_inches="tight")


def cold_step(cold, step):
    # Get files
    async_file = ""
    baseline_file = ""
    if cold == 0:
        async_file = "./result_cold/cold_async.txt"
        baseline_file = "./result_cold/cold_base.txt"
    else:
        async_file = "./result_warm/warm_async.txt"
        baseline_file = "./result_warm/warm_base.txt"

    async_data = read_file(async_file, cold, step)
    baseline_data = read_file(baseline_file, cold, step)

    plot(async_data, baseline_data, cold, step)


def extract_steps_from_bash_script(script_path):
    with open(script_path, 'r') as file:
        for line in file:
            if 'declare -a steps=' in line:
                # Extract the content between parentheses
                content = line.split('(')[1].split(')')[0]
                # Split by space and convert to integers
                steps = [int(num) for num in content.split()]
                return steps
    return None


# Set up argument parser
cold_list = [0, 1]
step_list = extract_steps_from_bash_script("./run.sh")

for c in cold_list:
    for s in step_list:
        cold_step(c, s)


