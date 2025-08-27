import argparse
from logging import log
import matplotlib.pyplot as plt
import re
from collections import defaultdict
import os

# Function to parse the data and calculate the ratio
def parse_and_plot(data, filename):
    # Regex pattern to match the header and subsequent digit lines
    header_pattern = re.compile(r"opt: (O[03]), len\(nop\): (\d+)")
    digit_pattern = re.compile(r"^\d+$")

    # Data structure to store parsed results
    results = defaultdict(lambda: defaultdict(list))

    lines = data
    i = 0
    while i < len(lines):
        header_match = header_pattern.match(lines[i])
        if header_match:
            opt, length = header_match.groups()
            length = int(length)

            # Parse the next two lines for digits
            if i + 2 < len(lines):
                line1 = lines[i + 1].strip()
                line2 = lines[i + 2].strip()

                if digit_pattern.match(line1) and digit_pattern.match(line2):
                    value1 = int(line1)
                    value2 = int(line2)

                    if value2 != 0:  # Avoid division by zero
                        ratio = value2 / value1
                        results[(opt)][length].append(ratio)

                i += 2  # Skip the digit lines
        i += 1

    # Plot the results
    for (opt), lengths in results.items():
        lengths = dict(sorted(lengths.items()))  # Sort by length
        x = list(lengths.keys())
        y = [sum(ratios) / len(ratios) for ratios in lengths.values()]  # Average ratio per length
        print(y)
        plt.figure()
        plt.plot(x, y, marker="o", label=f"{opt}")
        plt.xlabel("len(ADD)")
        plt.ylabel("Ratio (MTE on / MTE off)")
        for i, j in zip(x, y):
            plt.text(i, j, "%d" % i, ha="left")
        plt.title(f"Performance Ratio relative to ADD length")
        plt.xlim((0, 40))
        plt.legend()
        plt.grid(True)
        #plt.show()
        plt.savefig(f"{filename}-{opt}.pdf")

def main():
    parser = argparse.ArgumentParser(description='parse')
    parser.add_argument('-i', dest='input_path', help='file path')
    args = parser.parse_args()
    with open(args.input_path, "r") as f:
        lines = f.readlines()
        parse_and_plot(lines, os.path.splitext(args.input_path)[0])


if __name__ == "__main__":
    main()

