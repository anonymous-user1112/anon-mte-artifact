import numpy as np
import matplotlib.pyplot as plt

def read_file(fn):
    data = {}
    data[0] = {}
    data[-1] = {}
    data[-2] = {}
    size = 0
    test_mte = 0
    cpu = 0
    with open(fn) as f:
        for line in f:
            if("testmte" in line):
                test_mte = int(line.strip().split()[1])
            elif("buffer_size" in line):
                size = int(line.strip().split()[1])
                if(size not in data[test_mte]):
                    data[test_mte][size] = {}
            elif("Pin to CPU" in line):
                cpu = int(line.strip().split()[3])
                data[test_mte][size][cpu] = {}
            elif("Read after read to random memory location with dependency" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][size][cpu].setdefault("RR-Random-Dep", []).append(int(curr))
            elif("Write after write to random memory location no dependency" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][size][cpu].setdefault("WW-Random-noDep", []).append(int(curr))
            elif("Read after read to random memory location no dependency" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][size][cpu].setdefault("RR-Random-noDep", []).append(int(curr))
            elif("store to load forwarding" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][size][cpu].setdefault("Store-to-Load", []).append(int(curr))
            elif("Write after read to random memory location with dependency" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][size][cpu].setdefault("RW-Random-Dep", []).append(int(curr))
    f.close()
    return data

def plot(data, cpu, size):

    sync_mte = {}
    async_mte = {}
    no_mte = {}
    
    for s in data[0][size][cpu]:
        sync_mte[s] = (np.mean(data[0][size][cpu][s]), np.std(data[0][size][cpu][s]))
    
    for s in data[-1][size][cpu]:
        async_mte[s] = (np.mean(data[-1][size][cpu][s]), np.std(data[-1][size][cpu][s]))

    for s in data[-2][size][cpu]:
        no_mte[s] = (np.mean(data[-2][size][cpu][s]), np.std(data[-2][size][cpu][s]))

    print(sync_mte)
    print(async_mte)
    print(no_mte)

data = read_file("./out.txt")

plot(data, 0, 16777216)
plot(data, 4, 16777216)
plot(data, 8, 16777216)

