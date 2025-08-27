import numpy as np
import matplotlib.pyplot as plt
import os
import matplotlib.transforms as mtrans

def read_file(fn):
    data = []
    with open(fn) as f:
        for line in f:
            data.append(int(line))
            
    f.close()
    return data


def print_data(data):
    data.sort()
    data = data[10:90]
    print(np.mean(data), np.std(data))

def list_files_in_directory(directory):
    filename_data = {}
    for filename in os.listdir(directory):
        f = os.path.join(directory, filename)
        # checking if it is a file
        if os.path.isfile(f):
            data = read_file(f)
            filename_data[filename] = data
    return filename_data


def parser(all_data):
    core_barrier_flush_nop = {}
    for fn in all_data:
        barrier = fn.split("_")[2]
        
        nop = int(fn.split("_")[3].split("NOP")[1])
        
        flush = fn.split("_")[4]
        core = fn.split("_")[5]
        core_barrier_flush_nop.setdefault(core, {})
        print(barrier+" "+str(nop)+" "+flush+" "+core)
        if("FLUSHCACHE" in flush):
            core_barrier_flush_nop[core].setdefault("FLUSH", {})
            core_barrier_flush_nop[core]["FLUSH"].setdefault(barrier, {})
            core_barrier_flush_nop[core]["FLUSH"][barrier].setdefault(nop, {})
            if("FLUSHCACHETAG" in flush):
                mte_a = all_data[fn][:100]
                mte_a.sort()
                mte_a = mte_a[2:98]
                core_barrier_flush_nop[core]["FLUSH"][barrier][nop]["mte_async"] = (np.mean(mte_a), np.std(mte_a))
                
                mte = all_data[fn][100:]
                mte.sort()
                mte = mte[2:98]
                core_barrier_flush_nop[core]["FLUSH"][barrier][nop]["mte"] = (np.mean(mte), np.std(mte))
                
            else:
                no_mte = all_data[fn][:100]
                no_mte.sort()
                no_mte = no_mte[2:98]
                core_barrier_flush_nop[core]["FLUSH"][barrier][nop]["no_mte"] = (np.mean(no_mte), np.std(no_mte))
        else:            
            core_barrier_flush_nop[core].setdefault(flush, {})
            core_barrier_flush_nop[core][flush].setdefault(barrier, {})
            core_barrier_flush_nop[core][flush][barrier].setdefault(nop, {})
            no_mte = all_data[fn][:100]
            no_mte.sort()
            no_mte = no_mte[2:98]
            mte_a = all_data[fn][100:200]
            mte_a.sort()
            mte_a = mte_a[2:98]
            mte = all_data[fn][200:]
            mte.sort()
            mte = mte[2:98]
            core_barrier_flush_nop[core][flush][barrier][nop]["no_mte"] = (np.mean(no_mte), np.std(no_mte))
            core_barrier_flush_nop[core][flush][barrier][nop]["mte_async"] = (np.mean(mte_a), np.std(mte_a))
            core_barrier_flush_nop[core][flush][barrier][nop]["mte"] = (np.mean(mte), np.std(mte))
    return core_barrier_flush_nop

# def plot(core_barrier_flush_nop, core, flush, barrier):
#     plt.figure(figsize=(10, 6))
    
#     data = core_barrier_flush_nop[core][flush][barrier]
#     sorted_data = {key: data[key] for key in sorted(data.keys())}


#     avg_no_mte = [values["no_mte"][0] for values in sorted_data.values() if values]
#     std_no_mte = [values["no_mte"][1] for values in sorted_data.values() if values]
#     avg_mte = [values["mte"][0] for values in sorted_data.values() if values]
#     std_mte = [values["mte"][1] for values in sorted_data.values() if values]
    
#     plt.errorbar(sorted_data.keys(), avg_no_mte, yerr=std_no_mte, fmt='o-', capsize=5, label=barrier+"-nomte")
#     plt.errorbar(sorted_data.keys(), avg_mte, yerr=std_mte, fmt='o-', capsize=5, label=barrier+"-mtesync")
#     plt.legend()
#     plt.grid(True)
#     plt.savefig("%s-%s-%s.jpg" % (core, flush, barrier))

# # Example usage
# directory_path = "./out"  # Change this to your directory path
# all_data = list_files_in_directory(directory_path)
# core_barrier_flush_nop = parser(all_data)
# for core in core_barrier_flush_nop:  
#     for flush in core_barrier_flush_nop[core]:
#         for barrier in core_barrier_flush_nop[core][flush]:
#             plot(core_barrier_flush_nop, core, flush, barrier)
            

def plot(core_barrier_flush_nop, core, flush):
    fig, ax = plt.subplots(1, 1, figsize=(10, 6))

    minus = 1
    lw = 3
    for barrier in core_barrier_flush_nop[core][flush]:
        if("SY" in barrier):
            data = core_barrier_flush_nop[core][flush][barrier]
            sorted_data = {key: data[key] for key in sorted(data.keys())}

            avg_mte_a = [values["mte_async"][0] for values in sorted_data.values() if values]
            std_mte_a = [values["mte_async"][1] for values in sorted_data.values() if values]
            avg_no_mte = [values["no_mte"][0] for values in sorted_data.values() if values]
            std_no_mte = [values["no_mte"][1] for values in sorted_data.values() if values]
            avg_mte = [values["mte"][0] for values in sorted_data.values() if values]
            std_mte = [values["mte"][1] for values in sorted_data.values() if values]

            tr = mtrans.offset_copy(ax.transData, fig=fig, x=0.0, y=-minus, units='points')
            plt.errorbar(sorted_data.keys(), avg_no_mte, yerr=std_no_mte, capsize=1, label=barrier+"-nomte", linestyle='dotted', transform=tr, linewidth=lw)
            minus = minus + 1

            tr = mtrans.offset_copy(ax.transData, fig=fig, x=0.0, y=-minus, units='points')
            plt.errorbar(sorted_data.keys(), avg_mte_a, yerr=std_mte_a, capsize=1, label=barrier+"-mteasync", linestyle='dotted', transform=tr, linewidth=lw)
            minus = minus + 1

            tr = mtrans.offset_copy(ax.transData, fig=fig, x=0.0, y=-minus, units='points')
            plt.errorbar(sorted_data.keys(), avg_mte, yerr=std_mte, capsize=1, label=barrier+"-mtesync", linestyle='dotted', transform=tr, linewidth=lw)
            minus = minus + 1

            plt.legend()
            plt.grid(True)
    plt.xlabel("Number of NOPS")
    plt.ylabel("CPU cycle")
    plt.savefig("%s-%s.jpg" % (core, flush))

directory_path = "./out-nob-dmb"  # Change this to your directory path
all_data = list_files_in_directory(directory_path)
core_barrier_flush_nop = parser(all_data)
for core in core_barrier_flush_nop:  
    for flush in core_barrier_flush_nop[core]:
        plot(core_barrier_flush_nop, core, flush)