import os
import statistics
from typing import DefaultDict
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
import numpy as np
from rsf_parser import BenchmarkSuite
from matplotlib.ticker import MultipleLocator
import pprint


def median(values):
    if len(values) == 0:
        return None
    elif any(map(lambda x: x == None, values)):
        return None
    else:
        return statistics.median(values)


def average(values):
    if len(values) == 0:
        return None
    elif any(map(lambda x: x == None, values)):
        return None
    else:
        return sum(values) / len(values)


def get_summary_between_samples(l1: list, l2: list):
    A = np.array(l1)
    B = np.array(l2)
    ratios = A[:, None] / B[None, :]  # shape (len(A), len(B))
    norm_min = np.min(ratios)
    norm_max = np.max(ratios)
    norm_median = np.median(ratios)
    return norm_median, norm_min, norm_max


# merged data across logs that becomes one bar per each xlabel.
class PlotData:
    def __init__(self, files, label, suite="cpu2006"):
        self.bsuites = [BenchmarkSuite.from_log(f, suite_name=suite) for f in files]
        self.label = label

        d = DefaultDict(list)
        for bsuite in self.bsuites:  # each *.log file
            for bgroup in bsuite.groups:  # INT and FP
                for bm in bgroup.benchmarks:  # each prog
                    d[bm.name] += bm.times

        self.data = d
        self.summary = None
        self.err = None

    def get_summary(self, baseline, f):
        summary = {}
        err = {}
        mykeys = set(self.data.keys())
        if baseline is None:
            for k in mykeys:
                summary[k] = f(self.data[k])
                err[k] = statistics.stdev(self.data[k])
            return summary

        # when baseline is given
        baseline_keys = set(baseline.data.keys())
        common_keys = mykeys.intersection(baseline_keys)
        for k in sorted(common_keys):
            rmed, rmin, rmax = get_summary_between_samples(
                self.data[k], baseline.data[k]
            )
            summary[k] = rmed
            err[k] = [rmed - rmin, rmax - rmed]
        return summary, err

    def update_summary(self, baseline, f=statistics.median):
        self.summary, self.err = self.get_summary(baseline, f)


class CompareGroup:
    def __init__(self, files, label):
        dfs = [BenchmarkSuite.from_log(f).to_dict() for f in files]
        dfs = [self.extract_with_key(dfs[0], "average_time")] + [
            self.extract_with_key(d, "average_time") for d in dfs[1:]
        ]
        keys = [set(d.keys()) for d in dfs]
        allkeys = list(keys[0].intersection(*keys[1:]))
        self.base = {k: dfs[0][k] for k in allkeys}
        self.others = [{k: d[k] for k in allkeys} for d in dfs[1:]]
        self.label = label

    @staticmethod
    def extract_medians(d):
        # df = pd.DataFrame(columns=["name", "average_time"])
        df = {}
        for group in d["groups"]:
            for benchmark in group["benchmarks"]:
                # df.loc[len(df)] = [benchmark["name"], benchmark["average_time"]]
                df[benchmark["name"]] = benchmark["median_time"]

        return df

    @staticmethod
    def extract_with_key(d, key):
        df = {}
        for group in d["groups"]:
            for benchmark in group["benchmarks"]:
                df[benchmark["name"]] = benchmark[key]

        return df

    def normalize(self):
        normalized = []
        for d in self.others:
            for prog, time in self.base.items():
                d[prog] /= time
            normalized.append(d)
        self.others = normalized

    def dump_stat(self):
        for d in self.others:
            geomean = statistics.geometric_mean([r for r in d.values() if r > 0.1])
            maxd = max([r for r in d.values() if r > 0.1])
            mind = min([r for r in d.values() if r > 0.1])
            print(f"{self.label} - {mind:.2f} {maxd:.2f} {geomean:.3f}")


class PlotGroup:
    def __init__(self, plot_title, global_path=""):
        # self.cgs = []
        self.name = plot_title
        self.global_path = global_path
        self.pds = []

    def get_file_list(self, run_numbers, path, suite="cpu2006"):
        if suite == "cpu2006":
            return [ os.path.join(self.global_path, path, f"CPU2006.{str(n).zfill(3)}.log") for n in run_numbers ]
        elif suite == "cpu2017":
            return [ os.path.join(self.global_path, path, f"CPU2017.{str(n).zfill(3)}.log") for n in run_numbers ]

    def add_baseline_data(self, baseline_run_numbers, run_numbers, label, path="", suite="cpu2006"):
        # files = [ os.path.join(self.global_path, path, f"CPU2006.{str(n).zfill(3)}.log") for n in run_numbers ]
        # baseline_files = [ os.path.join(self.global_path, path, f"CPU2006.{str(n).zfill(3)}.log") for n in baseline_run_numbers ]
        files = self.get_file_list(run_numbers, path, suite=suite)
        baseline_files = self.get_file_list(baseline_run_numbers, path, suite=suite)
        pd = PlotData(files, label, suite=suite)
        baseline_pd = PlotData(baseline_files, label, suite=suite)
        pd.update_summary(baseline_pd)
        self.pds += [pd]
        return self

    def add_baseline(self, run_numbers, path="", suite="cpu2006"):
        return self.add_data(run_numbers, "baseline", path, suite=suite)

    def add_data(self, run_numbers, label, path="", suite="cpu2006"):
        # files = [ os.path.join(self.global_path, path, f"CPU2006.{str(n).zfill(3)}.log") for n in run_numbers ]
        files = self.get_file_list(run_numbers, path, suite=suite)
        pd = PlotData(files, label, suite=suite)
        self.pds += [pd]
        return self

    # def add_group(self, run_numbers, label, path=""):
    #     files = [os.path.join(self.global_path, path, f"CPU2006.{str(n).zfill(3)}.log") for n in run_numbers]
    #     cg = CompareGroup(files, label)
    #     self.cgs += [cg]
    #     return self

    colors = ["#762a83", "#af8dc3", "#e7d4e8", "#d9f0d3", "#7fbf7b", "#1b7837"]

    def plot(
        self,
        figsize=(9, 4),
        xlabel=None,
        ylabel="Normalized Time",
        suffix="",
        ylims=None,
        save_title=False,
        normalize=False,
        add_geomean=True,
        format="pdf",
        show_hline=False,
        bar_label=True,
        show_err=True,
        legend_ncols=3,
        colors=colors,
        ystride=0.5,
        legend_fontsize=16,
    ):

        print(self.name)
        fig, ax = plt.subplots(figsize=figsize)  # Adjust figure size as needed

        x_offset = 0  # Initialize offset for bars

        pds = self.pds
        if normalize:
            print("baseline >")
            pprint.pprint(pds[0].data)
            for pd in self.pds[1:]:
                pd.update_summary(self.pds[0])
            pds = self.pds[1:]

        xlabels = list(pds[0].data.keys())
        nbars = len(pds)
        bar_width = 0.84 / nbars
        # print(nbars, bar_width)
        for i, pd in enumerate(pds):
            if pd.summary is not None:
                record_to_plot = pd.summary.values()
            else:
                record_to_plot = pd.data.values()
            record_to_plot = list(record_to_plot)
            pprint.pprint(pd.data)
            if add_geomean:
                gm = statistics.geometric_mean([r for r in record_to_plot if r > 0.3])
                record_to_plot.append(gm)
            x = np.arange(len(record_to_plot)) + x_offset
            print(pd.data.keys())
            print(["{0:0.2f}".format(r) for r in record_to_plot])
            if show_err:
                yerr = np.array(list(pd.err.values()) + [[0, 0]])
                yerr = np.transpose(yerr)
                pprint.pprint(yerr)
                rects = ax.bar(
                    x,
                    record_to_plot,
                    yerr=yerr,
                    label=pd.label + suffix,
                    width=bar_width,
                    edgecolor="black",
                    color=colors[i],
                )
            else:
                rects = ax.bar(
                    x,
                    record_to_plot,
                    label=pd.label + suffix,
                    width=bar_width,
                    edgecolor="black",
                    color=colors[i],
                )
            if bar_label:
                ax.bar_label(rects, fmt="%.2f", fontsize="xx-small")

            x_offset += bar_width  # Increase offset for next set of bars

        if add_geomean:
            xlabels.append("geomean")
        pd = self.pds[0]

        if ylabel is not None:
            ax.set_ylabel("Norm. Runtime")
        if xlabel is not None:
            ax.set_xlabel(xlabel)
        if ylims is not None:
            ax.set_ylim(ylims)
            if ylims[0] == 0:
                ax.set_yticks(np.arange(0, ylims[1], 0.2))
        ax.yaxis.set_major_locator(MultipleLocator(ystride))
        ax.set_xticks(
            np.arange(len(xlabels)) + (nbars - 1) * bar_width / 2
        )  # Center x-axis ticks
        ax.set_xticklabels(
            xlabels, rotation=45, ha="right", rotation_mode="anchor"
        )  # Rotate labels if needed
        ax.set_xmargin(0.01)
        if show_hline:
            ax.axhline(y=1.0, color="black", linestyle="--")

        ax.legend(ncols=legend_ncols, frameon=True, framealpha=1, bbox_to_anchor=(1.01, 1.03), loc="upper right",
                  labelspacing=0.3, columnspacing=0.6, handletextpad=0.3)
        # plt.rc('font', size=SMALL_SIZE)          # controls default text sizes
        # plt.rc('axes', titlesize=SMALL_SIZE)     # fontsize of the axes title
        plt.rc("axes", labelsize=16)  # fontsize of the x and y labels
        plt.rc("xtick", labelsize=16)  # fontsize of the tick labels
        plt.rc("ytick", labelsize=16)  # fontsize of the tick labels
        plt.rc("legend", fontsize=legend_fontsize)  # legend fontsize
        plt.rc("figure", titlesize=8)  # fontsize of the figure title

        plt.tight_layout()  # Adjust layout to prevent labels from overlapping
        if save_title:
            ax.set_title(self.name)
        plt.savefig(f"{self.name}." + format, bbox_inches="tight")
        plt.close()
        # ax.set_title(self.name)
        # plt.show()

    def plot2(
        self,
        figsize=(9, 4),
        xlabel=None,
        ylabel="Normalized Time",
        suffix="",
        ylims=None,
        save_title=False,
        normalize=False,
        add_geomean=True,
        format="pdf",
        show_hline=False,
        bar_label=True,
        show_err=True,
        legend_ncols=3,
        use_broken_axis=False,
        y_break_low=None,
        y_break_high=None,
        gap_color="#808080",
        height_ratios=[0.45, 0.55],
    ):

        print(self.name)

        pds = self.pds
        if normalize:
            print("baseline >")
            pprint.pprint(pds[0].data)
            for pd in self.pds[1:]:
                pd.update_summary(self.pds[0])
            pds = self.pds[1:]

        xlabels = list(pds[0].data.keys())
        if add_geomean:
            xlabels.append("geomean")

        # M = number of categories, N = number of series
        M = len(xlabels)
        N = len(pds)
        x_base = np.arange(M)
        bar_width = 0.84 / N  # Dynamic bar width

        if use_broken_axis and y_break_low is not None and y_break_high is not None:
            # Create two subplots, sharing the x-axis
            fig, (ax_top, ax_bottom) = plt.subplots(
                2, 1, sharex=True, figsize=figsize, height_ratios=height_ratios
            )
            axes = [ax_top, ax_bottom]
        else:
            fig, ax = plt.subplots(figsize=figsize)
            axes = [ax]

        # --- Plotting loop ---
        for i, pd in enumerate(pds):
            # ... (your data preparation logic is the same)
            record_to_plot = list(
                pd.summary.values() if pd.summary else pd.data.values()
            )
            if add_geomean:
                gm = statistics.geometric_mean([r for r in record_to_plot if r > 0.3])
                record_to_plot.append(gm)

            # Use centered positioning instead of manual offset
            offset = (i - (N - 1) / 2) * bar_width
            positions = x_base + offset

            # Plot the same data on all available axes
            for ax in axes:
                yerr = None
                if show_err and hasattr(pd, "err"):
                    yerr_list = (
                        list(pd.err.values()) + [[0, 0]]
                        if add_geomean
                        else list(pd.err.values())
                    )
                    yerr = np.transpose(yerr_list)

                rects = ax.bar(
                    positions,
                    record_to_plot,
                    yerr=yerr,
                    label=pd.label + suffix,
                    width=bar_width,
                    edgecolor="black",
                    color=self.colors[i],
                )
                if bar_label:
                    ax.bar_label(rects, fmt="%.2f", fontsize="xx-small", padding=3)

        main_ax = axes[-1]  # The bottom axis is the primary one for labels

        if ylabel is not None:
            # Set a single y-label for the whole figure if using broken axes
            fig.text(
                -0.01, 0.5, "Norm. Runtime", va="center", rotation="vertical", size=16
            )

        if xlabel is not None:
            main_ax.set_xlabel(xlabel)

        main_ax.set_xticks(x_base)
        main_ax.set_xticklabels(
            xlabels, rotation=45, ha="right", rotation_mode="anchor"
        )
        main_ax.yaxis.set_major_locator(MultipleLocator(0.5))

        if use_broken_axis and y_break_low is not None and y_break_high is not None:
            ax_top.set_ylim(bottom=y_break_high)
            ax_top.yaxis.set_major_locator(MultipleLocator(0.5))
            ax_bottom.set_ylim(top=y_break_low)

            ax_top.set_xmargin(0.01)
            ax_bottom.set_xmargin(0.01)

            ax_top.spines["bottom"].set_visible(False)
            ax_bottom.spines["top"].set_visible(False)
            ax_top.tick_params(
                axis="x", which="both", bottom=False
            )  # Hide top x-tick lines

            # Add the diagonal "break" marks
            # d = .15
            # kwargs = dict(transform=ax_top.transAxes, color='k', clip_on=False)
            # ax_top.plot((-d, +d), (-d, +d), **kwargs)
            # ax_top.plot((1 - d, 1 + d), (-d, +d), **kwargs)
            # kwargs.update(transform=ax_bottom.transAxes)
            # ax_bottom.plot((-d, +d), (1 - d, 1 + d), **kwargs)
            # ax_bottom.plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)

            if show_hline:
                ax_top.axhline(y=1.0, color="black", linestyle="--")
                ax_bottom.axhline(y=1.0, color="black", linestyle="--")

        else:  # Original behavior
            if ylims is not None:
                main_ax.set_ylim(ylims)
            if show_hline:
                main_ax.axhline(y=1.0, color="black", linestyle="--")

        # Legends
        handles, labels = main_ax.get_legend_handles_labels()
        fig.legend(
            handles,
            labels,
            bbox_to_anchor=(0.9875, 0.925),
            ncols=legend_ncols,
            frameon=True,
            framealpha=1,
            labelspacing=0.3,
            columnspacing=0.6,
            handletextpad=0.3
        )
        # ax.legend(ncols=legend_ncols, frameon=True, framealpha=1)

        plt.rc("axes", labelsize=16)
        plt.rc("xtick", labelsize=16)
        plt.rc("ytick", labelsize=16)
        plt.rc("legend", fontsize=16)
        plt.rc("figure", titlesize=8)

        fig.tight_layout(rect=(.0, .0, 1.0, 0.95))

        # if save_title:
        #     fig.suptitle(self.name, y=1.08) # Use suptitle for the whole figure

        if use_broken_axis:
            fig.subplots_adjust(
                wspace=0.15, hspace=0.15
            )  # Adjust space between the two plots
            # Get the positions of the axes in figure coordinates
            pos_bottom = ax_bottom.get_position()
            pos_top = ax_top.get_position()

            # Define the rectangle coordinates for the gap
            rect = Rectangle(
                (pos_bottom.x0, pos_bottom.y1),  # (x, y)
                width=pos_bottom.width,  # width
                height=pos_top.y0 - pos_bottom.y1,  # height
                transform=fig.transFigure,
                facecolor=gap_color,
                edgecolor="none",
                clip_on=False,
                zorder=0,  # Put it in the background
            )
            fig.add_artist(rect)

        plt.savefig(f"{self.name}.{format}", bbox_inches="tight")
        plt.close()


if __name__ == "__main__":
    plt.style.use(style="seaborn-v0_8-whitegrid")
    params = {
        "text.usetex": True,
        "font.family": "serif",
        "font.weight": "semibold",
        # 'axes.titlesize': 12,
        # 'axes.labelsize': 10,
        # 'xtick.labelsize': 10,
        # 'ytick.labelsize': 10,
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

    one_per_core = [ '#bcbddc', '#a1d99b', '#fdae6b']
    two_per_core = [ '#756bb1', '#bcbddc', '#31a354', '#a1d99b', '#e6550d', '#fdae6b']
    ampere = ['#2171b5', '#6baed6', '#bdd7e7', '#eff3ff']

    # dicts = (PlotGroup("p8_ral")
    #         .add_baseline_data([1, 2], "A5"))

    # dicts = (PlotGroup("pixel9-ral-half", "results/pixel9/ral")
    #         .add_baseline_data([2, 10], "A520")
    #         .add_baseline_data([4, 11], "A720")
    #         .add_baseline_data([1, 9], "X4"))
    # dicts.plot()
    #
    # dicts = (PlotGroup("pixel9-ral-full", "results/pixel9/ral")
    #         .add_baseline_data([2, 7], "A520")
    #         .add_baseline_data([4, 8], "A720")
    #         .add_baseline_data([1, 5], "X4"))
    # dicts.plot()
    #
    # dicts = (PlotGroup("pixel9-glibc-heap-mte", "results/pixel9/glibc")
    #         .add_baseline_data([13, 14], "A520")
    #         .add_baseline_data([10, 12], "A720")
    #         .add_baseline_data([8, 9], "X4"))
    # dicts.plot()
    #
    # dicts = (PlotGroup("pixel8-glibc-heap-mte-sync", "results/pixel8/glibc")
    #         .add_baseline_data([101], [100], "A510")
    #         .add_baseline_data([99], [98], "A715")
    #         .add_baseline_data([97], [96], "X3"))
    # dicts.plot()
    # dicts = (PlotGroup("pixel8-glibc-mte-async")
    #         .add_baseline_data([189], [188], "A510", "results/pixel8/glibc/async")
    #         .add_baseline_data([191], [190], "A715", "results/pixel8/glibc/async")
    #         .add_baseline_data([193], [192], "X3", "results/pixel8/glibc/async"))
    # dicts.plot()
    #
    # dicts = (PlotGroup("pixel8-glibc")
    #         .add_baseline_data([101], [100], "Sync MTE (Little Core)", "results/pixel8/glibc")
    #         .add_baseline_data([189], [188], "Async MTE (Little Core)", "results/pixel8/glibc/async")
    #         .add_baseline_data([99], [98], "Sync MTE (Big Core)", "results/pixel8/glibc")
    #         .add_baseline_data([191], [190], "Async MTE (Big Core)", "results/pixel8/glibc/async")
    #         .add_baseline_data([97], [96], "Sync MTE (Perf Core)", "results/pixel8/glibc")
    #         .add_baseline_data([193], [192], "Async MTE (Perf Core)", "results/pixel8/glibc/async"))
    # dicts.plot_break_bar() #TODO
    #
    # dicts = (PlotGroup("pixel8-vtt", "results/pixel8/vtt")
    #         .add_baseline_data([146, 147], "A510")
    #         .add_baseline_data([148, 149], "A715")
    #         .add_baseline_data([141, 142], "X3"))
    # dicts.plot(ylims=(0.9, 1.1))
    #
    # dicts = (PlotGroup("pixel8-ifct+vtt", "results/pixel8/ifct+vtt")
    #         .add_baseline_data([165, 161], "A510")
    #         .add_baseline_data([164, 162], "A715")
    #         .add_baseline_data([163, 160], "X3"))
    # dicts.plot(ylims=(0.9, 1.1))
    #
    # dicts = (PlotGroup("pixel9-ral-v1 VS pixel8-ral-v2")
    #         .add_baseline_data([1, 9], "X4-half", "results/pixel9/ral")
    #         .add_baseline_data([1, 5], "X4-full", "results/pixel9/ral")
    #         .add_baseline_data([166, 167], "X3-half", "results/pixel8/ralv2")
    #         .add_baseline_data([166, 168], "X3-full", "results/pixel8/ralv2"))
    # dicts.plot()
    #
    # dicts = (PlotGroup("pixel8-A510-v1-vs-v2")
    #         .add_baseline_data([156, 157], "v1-half", "results/pixel8/ralv1")
    #         .add_baseline_data([156, 158], "v1-full", "results/pixel8/ralv1")
    #         .add_baseline_data([173, 174], "v2-half", "results/pixel8/ralv2")
    #         .add_baseline_data([173, 175], "v2-full", "results/pixel8/ralv2"))
    # dicts.plot()
    #
    # dicts = (PlotGroup("pixel8-ral-v2")
    #         .add_baseline_data([173, 174], "A510-half", "results/pixel8/ralv2")
    #         .add_baseline_data([173, 175], "A510-full", "results/pixel8/ralv2")
    #         .add_baseline_data([169, 170], "A715-half", "results/pixel8/ralv2")
    #         .add_baseline_data([169, 171], "A715-full", "results/pixel8/ralv2")
    #         .add_baseline_data([166, 167], "X3-half", "results/pixel8/ralv2")
    #         .add_baseline_data([166, 168], "X3-full", "results/pixel8/ralv2"))
    # dicts.plot()
    #
    #
    #
    # dicts = (PlotGroup("pixel8-ral-v3-st2g")
    #         .add_baseline_data([173, 207], "A510-half", "results/pixel8/ralv3")
    #         .add_baseline_data([173, 213], "A510-full", "results/pixel8/ralv3")
    #         .add_baseline_data([169, 208], "A715-half", "results/pixel8/ralv3")
    #         .add_baseline_data([169, 214], "A715-full", "results/pixel8/ralv3")
    #         .add_baseline_data([166, 209], "X3-half", "results/pixel8/ralv3")
    #         .add_baseline_data([166, 215], "X3-full", "results/pixel8/ralv3"))
    # dicts.plot()
    # SCS
    # 218: tagSS (cxc)
    # 219: tagSS (big)
    # 220: tagSS (little)
    #
    # 228: tagSS (big) (spike)
    # 230: tagSS (big) (more stable)
    # 240: tagSS (little)

    # 222: baseline (cxc)
    # 221: baseline (big) (spike)
    # 229: baseline (big) (more stable)
    # 238: baseline (little)

    # 227: NotagSS (cxc)
    # 231: NotagSS (big)
    # 241: NotagSS (little)
    #
    dicts = (
        PlotGroup("TagSS-Default")
        .add_baseline_data([238], [240], "Little Core", "results/pixel8/scs")
        .add_baseline_data([229], [230], "Big Core", "results/pixel8/scs")
        .add_baseline_data([222], [218], "Perf Core", "results/pixel8/scs")
    )
    dicts.plot(ylims=(0, 1.4), show_hline=True, bar_label=False, colors=one_per_core)

    dicts = (
        PlotGroup("pixel8-ral-v3")
        .add_baseline_data([173], [204], "Little Core", "results/pixel8/ralv3")
        .add_baseline_data([169], [205], "Big Core", "results/pixel8/ralv3")
        .add_baseline_data([166], [206], "Perf Core", "results/pixel8/ralv3")
    )
    dicts.plot(ylims=(0, 2.1), show_hline=True, bar_label=False, colors=one_per_core, ystride=0.2)

    # dicts = (PlotGroup("NoTag vs Tagged Shstk")
    #         .add_baseline_data([221], [228], "A715-NoTag", "results/pixel8/scs")
    #         .add_baseline_data([221], [219], "A715-TagSync", "results/pixel8/scs"))
    # dicts.plot(ylims=(0.7, 1.3))

    # dicts = (PlotGroup("NoTag vs Tagged Shstk")
    #         .add_baseline_data([229], [231], "Big-SS", "results/pixel8/scs")
    #         .add_baseline_data([229], [230], "Big-TagSS-sync", "results/pixel8/scs")
    #         .add_baseline_data([222], [227], "X3-SS", "results/pixel8/scs")
    #         .add_baseline_data([222], [218], "X3-TagSS-sync", "results/pixel8/scs"))
    # dicts.plot(ylims=(0.7, 1.3))

    # dicts = (
    #     PlotGroup("TagSS-SS")
    #     .add_baseline_data([241], [240], "Little Core", "results/pixel8/scs")
    #     .add_baseline_data([231], [230], "Big Core", "results/pixel8/scs")
    #     .add_baseline_data([227], [218], "Perf Core", "results/pixel8/scs")
    # )
    # dicts.plot(ylims=(0, 1.5), show_hline=True, bar_label=False, colors=one_per_core)
    #
    # dicts = (
    #     PlotGroup("SS-Default")
    #     .add_baseline_data([238], [241], "Little Core", "results/pixel8/scs")
    #     .add_baseline_data([229], [231], "Big Core", "results/pixel8/scs")
    #     .add_baseline_data([222], [227], "Perf Core", "results/pixel8/scs")
    # )
    # dicts.plot(ylims=(0, 1.4), show_hline=True, bar_label=False, colors=one_per_core)

    dicts = (
        PlotGroup("TagSS")
        .add_baseline_data([238], [241], "SS (Little Core)", "results/pixel8/scs")
        # .add_baseline_data([238], [240], "TagSS (Little Core)", "results/pixel8/scs")
        .add_baseline_data([238], [220], "TagSS (Little Core)", "results/pixel8/scs")
        .add_baseline_data([229], [231], "SS (Big Core)", "results/pixel8/scs")
        .add_baseline_data([229], [230], "TagSS (Big Core)", "results/pixel8/scs")
        # .add_baseline_data([229], [219], "TagSS (Big Core)", "results/pixel8/scs")
        .add_baseline_data([222], [227], "SS (Perf Core)", "results/pixel8/scs")
        .add_baseline_data([222], [218], "TagSS (Perf Core)", "results/pixel8/scs")
    )
    dicts.plot(ylims=(0, 1.6), show_hline=True, bar_label=False, colors=two_per_core, ystride=0.2)

    # sync VTT - phone2
    # 035: sync VTT little
    # 032: sync VTT big
    # 027: sync VTT X
    # *************** 037: sync Clang VTV little (unfortunately ran something wrong close to baseline)
    # *************** 036: sync Clang VTV big    (unfortunately ran something wrong close to baseline)
    # *************** 033: sync Clang VTV X  (unfortunately ran something wrong close to baseline)
    # 028: baseline X from phone 2

    # 041: Clang VTV X
    # 044: Clang VTV big
    # 053: Clang VTV little

    # 039: Clang IFCC X
    # 042: Clang IFCC big
    # **052: Clang IFCC little

    # 040: IFCT X
    # 043: IFCT big
    # 050: IFCT little

    # 049: baseline lto X
    # 048: baseline lto big
    # **051: baseline lto little

    dicts = (
        PlotGroup("VTT-vs-clang")
        .add_baseline_data(
            [51],
            [53],
            r"\texttt{cfi-vcall} (Little Core)",
            "results/pixel8/ifct+vtt-v2",
        )
        .add_baseline_data(
            [37], [35], "VTT (Little Core)", "results/pixel8/ifct+vtt-v2"
        )
        .add_baseline_data(
            [48], [44], r"\texttt{cfi-vcall} (Big Core)", "results/pixel8/ifct+vtt-v2"
        )
        .add_baseline_data([36], [32], "VTT (Big Core)", "results/pixel8/ifct+vtt-v2")
        .add_baseline_data(
            [49], [41], r"\texttt{cfi-vcall} (Perf Core)", "results/pixel8/ifct+vtt-v2"
        )
        .add_baseline_data([28], [27], "VTT (Perf Core)", "results/pixel8/ifct+vtt-v2")
    )
    dicts.plot(ylims=(0, 1.7), show_hline=True, bar_label=False, colors=two_per_core, legend_fontsize=14)

    dicts = (
        PlotGroup("IFCT-vs-clang")
        .add_baseline_data(
            [51],
            [52],
            r"\texttt{cfi-icall} (Little Core)",
            "results/pixel8/ifct+vtt-v2",
        )
        .add_baseline_data(
            [37], [50], "IFCT (Little Core)", "results/pixel8/ifct+vtt-v2"
        )
        .add_baseline_data(
            [48], [42], r"\texttt{cfi-icall} (Big Core)", "results/pixel8/ifct+vtt-v2"
        )
        .add_baseline_data([36], [43], "IFCT (Big Core)", "results/pixel8/ifct+vtt-v2")
        .add_baseline_data(
            [49], [39], r"\texttt{cfi-icall} (Perf Core)", "results/pixel8/ifct+vtt-v2"
        )
        .add_baseline_data([28], [40], "IFCT (Perf Core)", "results/pixel8/ifct+vtt-v2")
    )
    dicts.plot(ylims=(0, 1.7), show_hline=True, bar_label=False, colors=two_per_core, legend_fontsize=14)

    # dicts = (
    #     PlotGroup("VTT")
    #     .add_baseline_data([37], [35], "Little Core", "results/pixel8/ifct+vtt-v2")
    #     .add_baseline_data([36], [32], "Big Core", "results/pixel8/ifct+vtt-v2")
    #     .add_baseline_data([28], [27], "Perf Core", "results/pixel8/ifct+vtt-v2")
    # )
    # dicts.plot(ylims=(0, 1.5), show_hline=True, bar_label=False, colors=one_per_core)
    #
    # dicts = (
    #     PlotGroup("IFCT")
    #     .add_baseline_data([37], [50], "Little Core", "results/pixel8/ifct+vtt-v2")
    #     .add_baseline_data([36], [43], "Big Core", "results/pixel8/ifct+vtt-v2")
    #     .add_baseline_data([28], [40], "Perf Core", "results/pixel8/ifct+vtt-v2")
    # )
    # dicts.plot(ylims=(0, 1.5), show_hline=True, bar_label=False, colors=one_per_core)

    # 55: Tagbfi, X
    # 56: cfi-icall, X
    dicts = (
        PlotGroup("bfi+ldr-icall")
        # .add_data([], "Little Core", "results/pixel8/lto")
        # .add_data([], "Big Core", "results/pixel8/lto")
        .add_baseline([56], "results/pixel8/lto")
        .add_data([55], "Perf Core", "results/pixel8/lto")
    )
    dicts.plot(ylims=(0.9, 1.1), normalize=True, show_hline=True, bar_label=False, colors=one_per_core)

    ################ CPU2006 #################
    dicts = (
        PlotGroup("Pixel9_Perf_80p_Max")
        .add_baseline([13], "results/pixel9/glibcstatic")
        .add_data([14], "ASYNC", "results/pixel9/glibcstatic")
        .add_data([15], "SYNC", "results/pixel9/glibcstatic")
        .add_data([16], "ASYMM", "results/pixel9/glibcstatic")
    )
    dicts.plot2(
        normalize=True,
        show_hline=True,
        bar_label=False,
        use_broken_axis=True,
        y_break_high=8,
        y_break_low=3.4,
        height_ratios=[0.2, 0.8],
        legend_ncols=3,
    )

    dicts = (
        PlotGroup("Pixel9_Big_80p_Max")
        .add_baseline([9], "results/pixel9/glibcstatic")
        .add_data([10], "ASYNC", "results/pixel9/glibcstatic")
        .add_data([11], "SYNC", "results/pixel9/glibcstatic")
        .add_data([12], "ASYMM", "results/pixel9/glibcstatic")
    )
    dicts.plot(ylims=(0, 2.2), normalize=True, show_hline=True, bar_label=False, legend_ncols=3, ystride=0.25)

    dicts = (
        PlotGroup("Pixel9_Little_80p_Max")
        .add_baseline([5], "results/pixel9/glibcstatic")
        .add_data([6], "ASYNC", "results/pixel9/glibcstatic")
        .add_data([7], "SYNC", "results/pixel9/glibcstatic")
        .add_data([8], "ASYMM", "results/pixel9/glibcstatic")
    )
    dicts.plot(ylims=(0, 1.6), normalize=True, show_hline=True, bar_label=False, legend_ncols=3, ystride=0.25)

    dicts = (
        PlotGroup("Pixel8_Perf_80p_Max")
        .add_baseline([17], "results/pixel8/glibcstatic")
        .add_data([18], "ASYNC", "results/pixel8/glibcstatic")
        .add_data([19], "SYNC", "results/pixel8/glibcstatic")
        .add_data([20], "ASYMM", "results/pixel8/glibcstatic")
    )
    dicts.plot2(
        normalize=True,
        show_hline=True,
        bar_label=False,
        use_broken_axis=True,
        y_break_high=6.2,
        y_break_low=2.5,
        height_ratios=[0.25, 0.75],
        legend_ncols=3,
    )

    dicts = (
        PlotGroup("Pixel8_Big_80p_Max")
        .add_baseline([13], "results/pixel8/glibcstatic")
        .add_data([14], "ASYNC", "results/pixel8/glibcstatic")
        .add_data([15], "SYNC", "results/pixel8/glibcstatic")
        .add_data([16], "ASYMM", "results/pixel8/glibcstatic")
    )
    dicts.plot(normalize=True, show_hline=True, bar_label=False, legend_ncols=3, ystride=0.25)

    dicts = (
        PlotGroup("Pixel8_Little_80p_Max")
        .add_baseline([9], "results/pixel8/glibcstatic")
        .add_data([10], "ASYNC", "results/pixel8/glibcstatic")
        .add_data([11], "SYNC", "results/pixel8/glibcstatic")
        .add_data([12], "ASYMM", "results/pixel8/glibcstatic")
    )
    dicts.plot(ylims=(0, 1.6), normalize=True, show_hline=True, bar_label=False, legend_ncols=3, ystride=0.25)

    dicts = (
        PlotGroup("Pixel9_Perf_80p_Max_ANALOG")
        .add_baseline([13], "results/pixel9/glibcstatic")
        .add_data([14], "ASYNC", "results/pixel9/glibcstatic")
        .add_data([15], "SYNC", "results/pixel9/glibcstatic")
        .add_data([16], "ASYMM", "results/pixel9/glibcstatic")
        # .add_data([29], "Analog 1", "results/pixel9/analog_PROT_MTE")
        # .add_data([32], "Analog 2", "results/pixel9/analog_PROT_MTE"))
        .add_data([36], "Analog 1", "results/pixel9/analog")
        .add_data([39], "Analog 2", "results/pixel9/analog")
    )
    dicts.plot2(
        normalize=True,
        show_hline=True,
        bar_label=False,
        use_broken_axis=True,
        y_break_high=8,
        y_break_low=3.4,
        height_ratios=[0.2, 0.8],
        legend_ncols=2,
    )

    dicts = (
        PlotGroup("Pixel9_Big_80p_Max_ANALOG")
        .add_baseline([9], "results/pixel9/glibcstatic")
        .add_data([10], "ASYNC", "results/pixel9/glibcstatic")
        .add_data([11], "SYNC", "results/pixel9/glibcstatic")
        .add_data([12], "ASYMM", "results/pixel9/glibcstatic")
        # .add_data([28], "Analog 1", "results/pixel9/analog_PROT_MTE")
        # .add_data([31], "Analog 2", "results/pixel9/analog_PROT_MTE"))
        .add_data([35], "Analog 1", "results/pixel9/analog")
        .add_data([38], "Analog 2", "results/pixel9/analog")
    )
    dicts.plot(normalize=True, show_hline=True, bar_label=False, legend_ncols=2)

    dicts = (
        PlotGroup("Pixel9_Little_80p_Max_ANALOG")
        .add_baseline([5], "results/pixel9/glibcstatic")
        .add_data([6], "ASYNC", "results/pixel9/glibcstatic")
        .add_data([7], "SYNC", "results/pixel9/glibcstatic")
        .add_data([8], "ASYMM", "results/pixel9/glibcstatic")
        # .add_data([27], "Analog 1", "results/pixel9/analog_PROT_MTE")
        # .add_data([30], "Analog 2", "results/pixel9/analog_PROT_MTE"))
        .add_data([34], "Analog 1", "results/pixel9/analog")
        .add_data([37], "Analog 2", "results/pixel9/analog")
    )
    dicts.plot(normalize=True, show_hline=True, bar_label=False, legend_ncols=2)

    dicts = (
        PlotGroup("Pixel8_Perf_80p_Max_ANALOG")
        .add_baseline([17], "results/pixel8/glibcstatic")
        .add_data([18], "ASYNC", "results/pixel8/glibcstatic")
        .add_data([19], "SYNC", "results/pixel8/glibcstatic")
        .add_data([20], "ASYMM", "results/pixel8/glibcstatic")
        .add_data([30], "Analog 1", "results/pixel8/analog")
        .add_data([33], "Analog 2", "results/pixel8/analog")
    )
    dicts.plot2(
        normalize=True,
        show_hline=True,
        bar_label=False,
        use_broken_axis=True,
        y_break_high=6.2,
        y_break_low=3,
        height_ratios=[0.25, 0.75],
        legend_ncols=2,
    )

    dicts = (
        PlotGroup("Pixel8_Big_80p_Max_ANALOG")
        .add_baseline([13], "results/pixel8/glibcstatic")
        .add_data([14], "ASYNC", "results/pixel8/glibcstatic")
        .add_data([15], "SYNC", "results/pixel8/glibcstatic")
        .add_data([16], "ASYMM", "results/pixel8/glibcstatic")
        .add_data([29], "Analog 1", "results/pixel8/analog")
        .add_data([32], "Analog 2", "results/pixel8/analog")
    )
    dicts.plot(normalize=True, show_hline=True, bar_label=False, legend_ncols=2)

    dicts = (
        PlotGroup("Pixel8_Little_80p_Max_ANALOG")
        .add_baseline([9], "results/pixel8/glibcstatic")
        .add_data([10], "ASYNC", "results/pixel8/glibcstatic")
        .add_data([11], "SYNC", "results/pixel8/glibcstatic")
        .add_data([12], "ASYMM", "results/pixel8/glibcstatic")
        .add_data([28], "Analog 1", "results/pixel8/analog")
        .add_data([31], "Analog 2", "results/pixel8/analog")
    )
    dicts.plot(normalize=True, show_hline=True, bar_label=False, legend_ncols=2)

    dicts = (
        PlotGroup("AmpereOne_2600MHz")
        .add_baseline([1], "results/ampere1/glibc")
        .add_data([3], "SYNC", "results/ampere1/glibc")
    )
    dicts.plot(ylims=(0, 1.5), normalize=True, show_hline=True, bar_label=False, colors=["#af8dc3"], ystride=0.1)

    dicts = (
        PlotGroup("AmpereOne_2600MHz_ANALOG")
        .add_baseline([1], "results/ampere1/glibc")
        .add_data([3], "SYNC", "results/ampere1/glibc")
        .add_data([4], "Analog 1", "results/ampere1/glibc")
        .add_data([28], "Analog 2", "results/ampere1/glibc")
    )
    dicts.plot(ylims=(0, 2.4), normalize=True, show_hline=True, bar_label=False, colors=["#af8dc3", "#d9f0d3", "#7fbf7b"], ystride=0.25)

    dicts = (
        PlotGroup("AmpereOne_2600MHz_STLFWD_DISABLED")
        .add_baseline([35], "results/ampere1/glibc")
        .add_data([36], "SYNC", "results/ampere1/glibc")
    )
    dicts.plot(ylims=(0, 1.35), normalize=True, show_hline=True, bar_label=False, colors=["#af8dc3", "#d9f0d3", "#7fbf7b"], ystride=0.2)

    # dicts = (PlotGroup("AmpereOne_2600MHz_O3")
    #         .add_baseline([14], "results/ampere1/glibc")
    #         .add_data([15], "SYNC", "results/ampere1/glibc")
    #         .add_data([1], "O2", "results/ampere1/glibc")
    #         .add_data([3], "O2-SYNC", "results/ampere1/glibc"))
    # dicts.plot(ylims=(0, 1.5), normalize=True, show_hline=True, bar_label=False)

    dicts = (
        PlotGroup("AmpereOne_2600MHz_FWDCFI")
        .add_data([19], "baseline-lto", "results/ampere1/cfi")
        .add_data([18], r"\texttt{cfi-icall}", "results/ampere1/cfi")
        .add_data([17], r"IFCT w/ \texttt{bfi}", "results/ampere1/cfi")
    )
    dicts.plot(ylims=(0, 1.3), normalize=True, show_hline=True, bar_label=False, colors=ampere, ystride=0.2)

    dicts = (
        PlotGroup("AmpereOne_2600MHz_BWDCFI")
        .add_baseline(
            [20], "results/ampere1/cfi"
        )  # default-custom-glibc-libstdcxx-static
        .add_data([21], "SS", "results/ampere1/cfi")
        .add_data([22], "TagSS", "results/ampere1/cfi")
        .add_data([23], "RAL", "results/ampere1/cfi")
        # .add_data([24], "ral-full-tagcheck", "results/ampere1/cfi")
    )
    dicts.plot(
        ylims=(0, 1.7), normalize=True, show_hline=True, bar_label=False, legend_ncols=3, colors=ampere, ystride=0.2
    )

    dicts = (
        PlotGroup("AmpereOne_2600MHz_SPEC17_INTRATE_REF")
        .add_baseline([7], "results/ampere1/spec17", suite="cpu2017")
        .add_data([8], "SYNC", "results/ampere1/spec17", suite="cpu2017")
    )
    dicts.plot(normalize=True, show_hline=True, bar_label=False, colors=["#af8dc3"], ystride=0.1)

    dicts = (
        PlotGroup("AmpereOne_2600MHz_SPEC17_INTSPEED_REF")
        .add_baseline([3], "results/ampere1/spec17", suite="cpu2017")
        .add_data([4], "SYNC", "results/ampere1/spec17", suite="cpu2017")
    )
    dicts.plot(normalize=True, show_hline=True, bar_label=False, colors=["#af8dc3"], ystride=0.1)
