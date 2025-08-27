import numpy as np
import matplotlib.pyplot as plt



def read_file(fn):
    data = {}
    data[0] = {}
    data[-1] = {}
    data[1] = {}
    size = 0
    test_mte = 0
    cpu = 0
    with open(fn) as f:
        for line in f:
            if("mte" in line):
                test_mte = int(line.strip().split()[1])
            elif("buffer_size" in line):
                size = int(line.strip().split()[1])
            elif("Pin to CPU" in line):
                cpu = int(line.strip().split()[3])
                if(cpu not in data[test_mte]):
                    data[test_mte][cpu] = {}
                data[test_mte][cpu][size] = {}
            elif("Read after read to random memory location with dependency" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("RR-Random-Dep", []).append(float(curr)/float(size))
            elif(("Write after write to random memory location no dependency" in line) and ("+" not in line)):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("WW-Random-noDep", []).append(float(curr)/float(size))
            elif("MTE + DSB ST: Write after write to random memory location no dependency" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("WW-Random-noDep-DSB-ST", []).append(float(curr)/float(size))
            elif("MTE + DMB ST: Write after write to random memory" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("WW-Random-noDep-DMB-ST", []).append(float(curr)/float(size))
            elif("Read after read to random memory location no dependency" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("RR-Random-noDep", []).append(float(curr)/float(size))
            elif(("store to load forwarding" in line) and ("+" not in line)):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("Store-to-Load", []).append(float(curr)/float(size))
            elif("MTE + DSB: store to load forwarding" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("Store-to-Load-DSB", []).append(float(curr)/float(size))
            elif("MTE + DMB: store to load forwarding" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("Store-to-Load-DMB", []).append(float(curr)/float(size))
            elif("Read after read sequential" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("RR-seq", []).append(float(curr)/float(size))
            elif("Write after write sequential" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("WW-seq", []).append(float(curr)/float(size))
            elif("Read after write sequential" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("RW-seq", []).append(float(curr)/float(size))
            
    f.close()
    return data


def read_sve_file(fn):
    data = {}
    data[0] = {}
    data[-1] = {}
    data[1] = {}
    size = 0
    test_mte = 0
    cpu = 0
    with open(fn) as f:
        for line in f:
            if("testmte" in line):
                test_mte = int(line.strip().split()[1])
            elif("buffer_size" in line):
                size = int(line.strip().split()[1])
            elif("Pin to CPU" in line):
                cpu = int(line.strip().split()[3])
                if(cpu not in data[test_mte]):
                    data[test_mte][cpu] = {}
                data[test_mte][cpu][size] = {}
            elif("Vector streaming load" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("Stream-load", []).append(float(curr)/float(size))
            elif("Vector streaming store" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("Stream-store", []).append(float(curr)/float(size))
            elif("Vector gather load" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("Gather-load", []).append(float(curr)/float(size))
            elif(("Vector scatter store" in line) and ("+" not in line)):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("Scatter-store", []).append(float(curr)/float(size))
            elif("Vector scatter store + DMB ST" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("Scatter-store-DMB", []).append(float(curr)/float(size))
            elif("Vector scatter store + DSB ST" in line):
                curr = float(line.strip().split()[-2])
                data[test_mte][cpu][size].setdefault("Scatter-store-DSB", []).append(float(curr)/float(size))
    f.close()
    return data

# For a given workload and cpu pair, plot size vs performance
def plot(data, cpu, workload, workload_base, baseline):
    sync_mte = {}
    async_mte = {}
    no_mte = {}
    
    if(baseline == 0):
        for s in data[1][cpu]:
            sync_mte[s] = data[1][cpu][s][workload].copy()
    else:
        for s in data[1][cpu]:
            sync_mte[s] = data[1][cpu][s][workload_base].copy()
    
    for s in data[-1][cpu]:
        async_mte[s] = data[-1][cpu][s][workload].copy()
    
    if(baseline == 0):
        for s in data[0][cpu]:
            no_mte[s] = data[0][cpu][s][workload].copy()

    sync_mte_stat = {}
    async_mte_stat = {}

    for s in sync_mte:
        temp_async = []
        temp_sync = []
        if(baseline == 0):
            for d in range(len(no_mte[s])):
                temp_sync.append(sync_mte[s][d]/no_mte[s][d])
                temp_async.append(async_mte[s][d]/no_mte[s][d])
        else:
            for d in range(len(async_mte[s])):
                temp_sync.append(sync_mte[s][d]/async_mte[s][d])
                temp_async.append(1)
        temp_sync.sort()
        temp_sync = temp_sync[10:90]
        sync_mte_stat[s] = (np.mean(temp_sync), np.std(temp_sync))
        temp_async.sort()
        temp_async = temp_async[10:90]
        async_mte_stat[s] = (np.mean(temp_async), np.std(temp_async))

    print(sync_mte_stat)
    print(async_mte_stat)
    # Combine the dictionaries for easy plotting
    dicts = [async_mte_stat, sync_mte_stat]
    labels = ['ASYNC MTE', 'SYNC MTE']

    sizes = list(sync_mte_stat.keys())
    size_values = range(len(sizes))  # Numeric x-axis for categorical sizes

    # Plot
    plt.figure(figsize=(10, 6))
    for i, d in enumerate(dicts):
        averages = [d[size][0] for size in sizes]
        std_devs = [d[size][1] for size in sizes]
        
        plt.errorbar(size_values, averages, yerr=std_devs, fmt='o-', capsize=5, label=labels[i])

    # Customize plot
    plt.xticks(size_values, sizes, rotation=45)  # Set x-axis labels to sizes
    plt.xlabel('Size')
    plt.ylabel('Average')
    plt.title('Size vs Average with Error Bars')
    plt.legend()
    plt.grid(True)
    plt.savefig("%s-%d.pdf" % (workload, cpu))

data = read_file("./out_bench_8.txt")
cpu = 8
baseline = 0

workload = "RR-Random-Dep"
workload_base = "Store-to-Load"
plot(data, cpu, workload, workload_base, baseline)

workload = "WW-Random-noDep"
plot(data, cpu, workload, workload_base, baseline)

workload = "RR-Random-noDep"
plot(data, cpu, workload, workload_base, baseline)

workload = "Store-to-Load"
plot(data, cpu, workload, workload_base, baseline)

workload = "RR-seq"
plot(data, cpu, workload, workload_base, baseline)

workload = "RW-seq"
plot(data, cpu, workload, workload_base, baseline)

workload = "WW-seq"
plot(data, cpu, workload, workload_base, baseline)

baseline = -1

workload = "Store-to-Load-DSB"
workload_base = "Store-to-Load"
plot(data, cpu, workload, workload_base, baseline)

workload = "Store-to-Load-DMB"
workload_base = "Store-to-Load"
plot(data, cpu, workload, workload_base, baseline)

workload = "WW-Random-noDep-DSB-ST"
workload_base = "WW-Random-noDep"
plot(data, cpu, workload, workload_base, baseline)

workload = "WW-Random-noDep-DMB-ST"
workload_base = "WW-Random-noDep"
plot(data, cpu, workload, workload_base, baseline)
