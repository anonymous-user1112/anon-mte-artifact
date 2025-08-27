import argparse
import statistics
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, FixedLocator
from matplotlib.transforms import Affine2D
from collections import defaultdict
import os

core_labels = { "pixel8" : [ "A510", "A715", "X3" ],
               "pixel9" : [ "A520", "A720", "X4" ] }

def load_data(input_path):
    with open(input_path, 'r') as f:
        data = f.read()
        lines = data.split('\n')
    return lines

def summarise(input_path):
    times = defaultdict(list)
    ext_name = ""
    lines = load_data(input_path)
    results_label = "spec.cpu2006.results"
    ext_label = "spec.cpu2006.ext"
    for line in lines:
        if results_label in line:
            if ".reported_time" in line:
                reported_time = line.split()[-1]
                s = line.split('.')
                name = s[3]
                iter = s[-3]
                #print(iter, reported_time)
                try:
                    times[name].append(float(reported_time))
                except:
                    print("Error parsing line: " + line + ". Fragment: " + reported_time)
                    times[name].append(1)
        if ext_label in line:
                ext_name = line.split()[1]

    avg_times = {}
    for k, v in times.items():
        avg_times[k] = statistics.mean(v)

    return (ext_name, avg_times)

def get_merged_summary(result_path, n):
    int_input_path = f"{result_path}/CINT2006.{str(n).zfill(3)}.ref.rsf"
    name1,int_times = summarise(int_input_path)

    times = {}
    times.update(int_times)

    fp_input_path  = f"{result_path}/CFP2006.{str(n).zfill(3)}.ref.rsf"
    name2 = ""
    if os.path.exists(fp_input_path):
        name2,fp_times  = summarise(fp_input_path)
        times.update(fp_times)
    assert( (not (name1 != "" and name2 == "")) and (name1 == name2) or (name1 == "") or (name2 == ""))

    return name1,times

def normalize(times, baseline_ext):
    normalized_times = {}

    for core, per_core_results in times.items():
        per_core_norm = defaultdict(dict)
        baseline_times = times[core][baseline_ext]
        for ext_name, per_ext_results in per_core_results.items():
            if ext_name == baseline_ext:
                continue
            for prog_name, time in per_ext_results.items():
                base_time = baseline_times[prog_name]
                per_core_norm[ext_name][prog_name] = time / base_time

        normalized_times[core] = per_core_norm

    return normalized_times

def all_times_to_vals(all_times):
    vals = []
    for d in all_times.values():
        l = sorted(list(d.items()),key=lambda x: x[0])
        ll = [v for (k,v) in l]
        vals.append(ll)
    return vals

def make_graph(all_times, output_path, machine, use_percent=False, ymin=1.0, ymax=2.0):
    # (core, ext, prog)

    fig = plt.figure(figsize=(12,4))
    sample_per_core = next(iter(all_times.values()))
    sample_per_ext = next(iter(sample_per_core.values()))
    cores = list(all_times.keys())
    exts = list(sample_per_core.keys())
    progs = list(sample_per_ext.keys())
    num_cores = len(cores)
    num_exts = len(exts)
    num_progs = len(progs) # get any element

    output_filename = machine + "-" + "_".join(exts)

    plt.rcParams['pdf.fonttype'] = 42 # true type font
    # plt.rcParams['font.family'] = 'serif'
    # plt.rcParams['font.serif'] = ['Times New Roman'] + plt.rcParams['font.serif']
    plt.rcParams['font.size'] = '11'
    # https://colorbrewer2.org/#type=diverging&scheme=Spectral&n=5
    colors = ['#D7191C', '#2B83BA', '#FDAE61', '#ABDDA4', '#FFFFBF']
    # NOTE: gpt recommended
    # colors = ['#8ECAE6','#219EBC','#F8B195','#F67280','#95D5B2','#40916C']

    # Set the positions for the bars
    n_groups = num_cores * num_exts
    x = np.arange(num_progs)
    width = (1.0 / ( (n_groups) + 1))        # the width of the bars
    total_width = width * n_groups

    rects = []
    i = 0
    for core in cores:
        for ext in exts:
            bar_values = [all_times[core][ext][prog] for prog in progs]
            print(core, ext, statistics.geometric_mean(bar_values))
            print(core, ext, statistics.median(bar_values))
            label = f'{core}-{ext}'
            rects += plt.bar(x + (i - n_groups/2) * width, bar_values, width, label=label, color=colors[i])
            i += 1

    # Customize the plot
    # plt.axhline(y=1.0, color='black', linestyle='dashed')
    if use_percent:
        plt.ylabel('Norm. runtime')
    else:
        plt.ylabel('Relative execution time')
    plt.xticks(x, progs, rotation=45, ha='right', rotation_mode='anchor')
    plt.grid(True, axis='y', linestyle='--', alpha=0.5)
    plt.margins(0.01, 0.01)

    ax = plt.gca()
    if use_percent:
        ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0%}'.format(y)))
        # ax.yaxis.set_major_locator(FixedLocator(np.arange(-.5,10,.5)))
    else:
        ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.2f}×'.format(y)))
        ax.yaxis.set_major_locator(FixedLocator(list([0, 0.25, 0.50, 0.75, 1, 1.25, 1.5, 1.75, 2.0])))

    ax.legend(ncol=3, loc='upper right', bbox_to_anchor=(1, 1))

    plt.ylim(bottom=ymin, top=ymax)

    # Adjust layout to prevent label cutoff
    plt.tight_layout()

    plt.savefig(os.path.join(output_path, output_filename + ".pdf"), format="pdf", bbox_inches="tight", pad_inches=0)

    # output_stats = os.path.join(output_path, "output.stats")
    # if os.path.exists(output_stats):
    #     os.remove(output_stats)
    #
    # with open(output_stats, "a+") as myfile:
    #     myfile.write(f"Benchmarks: {abels}\n")
    #
    # for i in range(num_exts):
    #     result_geomean = statistics.geometric_mean(vals[i])
    #     result_median = statistics.median(vals[i])
    #     result_min = min(vals[i])
    #     result_max = max(vals[i])
    #     with open(output_stats, "a+") as myfile:
    #         myfile.write(f"{mitigations[i]} geomean = {result_geomean} {mitigations[i]} median = {result_median} min = {result_min} max = {result_max} raw_values = {[p*100 for p in vals[i]]}\n")


def main():
    parser = argparse.ArgumentParser(description='Graph Spec Results')
    parser.add_argument('-i', dest='input_path', help='file path')
    parser.add_argument('-m', dest='machine', help='(pixel8|pixel9)')
    # parser.add_argument('-n', dest='nth_run', help='CINT2006.(\\d+).ref.rsf')
    parser.add_argument('-l', dest='little', help='a comma separated list')
    parser.add_argument('-b', dest='big', help='a comma separated list')
    parser.add_argument('-x', dest='ultra', help='a comma separated list')
    parser.add_argument('--ymin', dest='ymin', help='y-axis min')
    parser.add_argument('--ymax', dest='ymax', help='y-axis max')
    parser.add_argument('--label', dest='label', help='give alternative extension names')
    # parser.add_argument('--baseline', dest='baseline', default=None, help='set the baseline for comparison')
    parser.add_argument('--usePercent', dest='usePercent', default=False, action='store_true')
    args = parser.parse_args()

    print(args.input_path)
    print(args.machine)
    print(args.little)
    print(args.big)
    print(args.ultra)
    labels = []
    use_given_name = False
    if args.label is not None:
        labels += args.label.split(',')
        assert(len(labels) == len(args.little.split(',')))
        use_given_name = True

    # I expect this to be unordered, respecting comma separated input order
    all_data = defaultdict(dict)
    baseline = None
    for i, per_core in enumerate([args.little, args.big, args.ultra]):
        if per_core is None: continue
        core_label = core_labels[args.machine][i]
        for j, bench in enumerate(per_core.split(',')):
            ext_name, data = get_merged_summary(args.input_path, bench)
            if use_given_name:
                ext_name = labels[j]
            # print(ext_name, data)
            assert(ext_name not in all_data[core_label])
            all_data[core_label][ext_name] = data
            if baseline is None:
                baseline = ext_name

    # print(all_data)
    normalized_times = normalize(all_data, baseline)

    print(normalized_times)
    make_graph(normalized_times, args.input_path, args.machine, args.usePercent, float(args.ymin), float(args.ymax));

    # You can then plot the data using matplotlib or seaborn as needed


if __name__ == "__main__":
    main()
