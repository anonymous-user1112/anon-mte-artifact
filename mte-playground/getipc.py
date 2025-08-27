import re
import sys
import csv
from collections import defaultdict
from pathlib import Path

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <perf_output_file>")
    sys.exit(1)

input_file = sys.argv[1]

mode = {"0":"disabled", "1": "async", "3": "sync", "5": "asymm"}

# Improved regex to match perf command lines
header_re = re.compile(
    r" Performance counter stats for 'env\s+(GLIBC_TUNABLES=[^ ]+)\s+([^\s']+)"
)

cycles_re = re.compile(r'([\d,]+)\s+cycles')
instructions_re = re.compile(r'([\d,]+)\s+instructions')

# Accumulate stats
data = defaultdict(lambda: {'cycles': 0, 'instructions': 0, 'count': 0})

with open(input_file) as f:
    env = prog = None
    for line in f:
        header_match = header_re.match(line)
        if header_match:
            env = header_match.group(1).lstrip("GLIBC_TUNABLES=glibc.mem.tagging=")
            env = mode[env]
            prog = Path(header_match.group(2)).name  # strip directory
            continue

        if env and prog:
            cycle_match = cycles_re.search(line)
            if cycle_match:
                val = int(cycle_match.group(1).replace(',', ''))
                data[(env, prog)]['cycles'] += val
                continue

            instr_match = instructions_re.search(line)
            if instr_match:
                val = int(instr_match.group(1).replace(',', ''))
                data[(env, prog)]['instructions'] += val
                data[(env, prog)]['count'] += 1  # Count complete samples
                continue



# Write CSV
output_file = Path(input_file).stem + ".summary.csv"
with open(output_file, mode='w', newline='') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(['program', 'env', 'cycles', 'instructions', 'instructions/cycles'])

    for (env, prog), stats in sorted(data.items()):
        cycles = stats['cycles']
        instructions = stats['instructions']
        ipc = instructions / cycles if cycles else 0
        writer.writerow([prog, env, cycles, instructions, f"{ipc:.2f}"])
