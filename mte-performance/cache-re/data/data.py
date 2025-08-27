import numpy as np
import matplotlib.pyplot as plt

def read_file(fn):
    data = {}
    size = 0
    cpu = 0
    with open(fn) as f:
        for line in f:
            if("buffer_size" in line):
                size = int(line.strip().split()[1])
                
                if(size not in data):
                    data[size] = {}
            elif("Pin to CPU" in line):
                cpu = int(line.strip().split()[3])
                data[size][cpu] = {}
            elif("Pointer Chasing: " in line):
                curr = float(line.strip().split()[-2])
                data[size][cpu].setdefault("Cycles", []).append(float(curr)/float(size))
           
    f.close()
    return data

def plot(data, cpu, name):
    # trim data
    data_single = {}
    for s in data:
        data_single[s] = data[s][cpu]["Cycles"].copy()

    data_stat = {}
    for s in data_single:
        data_single[s].sort()
        data_single[s] = data_single[s][10:490]
        data_stat[s] = (np.mean(data_single[s]), np.std(data_single[s]))
    

    print(data_stat)

    # Combine the dictionaries for easy plotting
    sizes = list(data_stat.keys())
    averages = [data_stat[size][0] for size in sizes]
    std_devs = [data_stat[size][1] for size in sizes]

    # Plotting
    plt.figure(figsize=(10,6))
    plt.errorbar(sizes, averages, yerr=std_devs, fmt='o', capsize=5, label='Average with std deviation')
    plt.xlabel('Size')
    plt.ylabel('Average')
    plt.title(name)
    plt.legend()
    plt.grid(True)
    plt.savefig("%s.pdf" % (name))

data = read_file("./cpu_5_l2_two_core.txt")
plot(data, 5 , "cpu_5_l2_two_core")
