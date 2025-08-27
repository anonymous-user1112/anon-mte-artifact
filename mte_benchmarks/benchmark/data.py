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
            elif("Read after read sequential" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][size][cpu].setdefault("RR-seq", []).append(int(curr))
            elif("Write after write sequential" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][size][cpu].setdefault("WW-seq", []).append(int(curr))
            elif("Read after write sequential" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][size][cpu].setdefault("RW-seq", []).append(int(curr))
            elif("Starting SVE" in line):
                break
    f.close()
    return data

# For a given workload and cpu pair, plot size vs performance
def plot(data, cpu, workload):
    sync_mte = {}
    async_mte = {}
    no_mte = {}

    
    for s in data[0]:
        sync_mte[s] = (np.mean(data[0][s][cpu][workload]), np.std(data[0][s][cpu][workload]))
    
    for s in data[-1]:
        async_mte[s] = (np.mean(data[-1][s][cpu][workload]), np.std(data[-1][s][cpu][workload]))
    
    for s in data[-2]:
        no_mte[s] = (np.mean(data[-2][s][cpu][workload]), np.std(data[-2][s][cpu][workload]))

    print(sync_mte)
    print(async_mte)
    print(no_mte)

    # Combine the dictionaries for easy plotting
    dicts = [no_mte, async_mte, sync_mte]
    labels = ['No MTE', 'ASYNC MTE', 'SYNC MTE']

    sizes = list(no_mte.keys())
    size_values = range(len(sizes))  # Numeric x-axis for categorical sizes

    # Plot
    plt.figure(figsize=(10, 6))
    for i, d in enumerate(dicts):
        averages = [d[size][0] for size in sizes]
        std_devs = [d[size][1] for size in sizes]
        
        plt.errorbar(size_values, averages, yerr=std_devs, fmt='o-', capsize=5, label=labels[i])

    # Customize plot
    plt.yscale('log')
    plt.xticks(size_values, sizes, rotation=45)  # Set x-axis labels to sizes
    plt.xlabel('Size')
    plt.ylabel('Average')
    plt.title('Size vs Average with Error Bars')
    plt.legend()
    plt.grid(True)
    plt.savefig("%s-%d.pdf" % (workload, cpu))


data = read_file("./out_bench.txt")

workload = "RW-seq"
plot(data, 0, workload)
plot(data, 4, workload)
plot(data, 8, workload)