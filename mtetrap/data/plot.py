from ctypes import sizeof
import argparse
import matplotlib.pyplot as plt
import re
from collections import defaultdict
import os
import numpy as np

openssl_data = """
default 512 0.000034 0.000002
default 1024 0.000151 0.000007
default 2048 0.001002 0.000025
default 3072 0.002977 0.000054
default 4096 0.006616 0.000095
default 7680 0.051917 0.000329
default 15360 0.298824 0.001304
mtekmod 512 0.000048 0.000018
mtekmod 1024 0.000186 0.000037
mtekmod 2048 0.001057 0.000087
mtekmod 3072 0.003052 0.000149
mtekmod 4096 0.006722 0.000209
mtekmod 7680 0.052094 0.000558
mtekmod 15360 0.299412 0.001802
mtesig 512 0.000051 0.000022
mtesig 1024 0.000191 0.000046
mtesig 2048 0.001056 0.000115
mtesig 3072 0.003057 0.000193
mtesig 4096 0.006744 0.000248
mtesig 7680 0.052011 0.000606
mtesig 15360 0.299706 0.001786
mprotsig 512 0.000104 0.000061
mprotsig 1024 0.000287 0.000113
mprotsig 2048 0.001271 0.000235
mprotsig 3072 0.003338 0.000373
mprotsig 4096 0.007147 0.000535
mprotsig 7680 0.052416 0.001075
mprotsig 15360 0.300909 0.002822
"""

def parse_openssl_data(data):
  lines = data
  x_values = {}
  d_sign = {}
  d_verify = {}
  for line in lines:
    # regex line pattern for openssl_data string
    line_pattern = re.compile(r"^([a-z]+) (\d+) (\d+\.\d+) (\d+\.\d+)$")
    line_match = line_pattern.match(line)
    if not line_match: continue
    suite, sz, sign, verify = line_match.groups()
    sign = float(sign) * 10e6
    verify = float(verify) * 10e6
    print(sign, verify)

    x_values[int(sz) / 8] = True
    d_sign[suite] = d_sign.get(suite, [])
    d_sign[suite].append(sign)
    d_verify[suite] = d_verify.get(suite, [])
    d_verify[suite].append(verify)

  plt.figure(figsize=(5, 2))

  labels = ['Baseline', 'MTE-kernel-tracer', 'MTE-tracer', 'Page-tracer']

  for i, (k, v) in enumerate(d_sign.items()):
    if labels[i] == 'Baseline': continue
    normalized_y = [y / base_y for y, base_y in zip(v, d_sign["default"])]
    plt.plot(x_values.keys(),  normalized_y, label=labels[i])

  # plt.title("Openssl RSA_sign() with Data Tracing")
  #plt.legend(ncols=2, frameon=True, framealpha=1)
  plt.xlabel("Buffer Size (bytes)\nRSA Sign.")
  plt.ylabel("Norm. Runtime")
  plt.yticks(np.arange(1, 3.5, 0.5))
  plt.xlim(64, 1024)
  plt.ylim(bottom=0.8)
  plt.legend(frameon=True, framealpha=1)
  plt.grid()
  plt.tight_layout()
  plt.savefig(f"rsa_sign.pdf", bbox_inches='tight')

  plt.figure(figsize=(5, 2))

  for i, (k, v) in enumerate(d_verify.items()):
    if labels[i] == 'Baseline': continue
    normalized_y = [y / base_y for y, base_y in zip(v, d_verify["default"])]
    plt.plot(x_values.keys(), normalized_y, label=labels[i])

  # plt.title("Openssl RSA_verify() with Data Tracing")
  plt.xlabel("Buffer Size (bytes)\nRSA Verify.")
  plt.ylabel("Norm. Runtime")
  yticks = np.arange(0, 35, 5)
  yticks[0] = 1
  plt.yticks(yticks)
  plt.xlim(64, 1024)
  plt.ylim(bottom=0.8)
  plt.legend(frameon=True, framealpha=1)
  plt.grid()
  plt.tight_layout()
  plt.savefig(f"rsa_verify.pdf", bbox_inches='tight')


# plt.style.use(style='seaborn-v0_8-whitegrid')
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
        \usepackage[libertine]{newtxmath}
        \usepackage{libertine}
        \usepackage{zi4}
        """,
        }
plt.rcParams.update(params)
plt.rc('axes', labelsize=12)    # fontsize of the x and y labels
plt.rc('xtick', labelsize=12)    # fontsize of the tick labels
plt.rc('ytick', labelsize=12)    # fontsize of the tick labels
plt.rc('legend', fontsize=12)    # legend fontsize
plt.rc('figure', titlesize=8)  # fontsize of the figure title
parse_openssl_data(openssl_data.split("\n"))
