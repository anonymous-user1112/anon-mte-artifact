import numpy as np

def read_file(fn):
    data = []
    with open(fn) as f:
        for line in f:
            c = line.strip().split()[3]
            data.append(int(c))
    f.close()
    print(fn + " " + str(np.mean(data)))

read_file("bound.txt")
read_file("bound_mte.txt")
read_file("guard.txt")
read_file("guard_mte.txt")