- `build_spec_utils.sh` : put this inside *SPEC* directory
- `spectools.patch ` : put this inside *SPEC* directory
- `llvm-aarch64-mte-tagcfi.cfg `: SPEC config file
- `bench_runner` : a script that helps pinning the core and fixing its frequency (pixel8-preset).
- `plot_all.py` and `rsf_parser.py` : plotting tool
- `plot_ref.py` : outdated
- `results/` : spec result directory 
- `specenv/` : Nix user only

## Environment setup : Building SPEC

```sh
mkdir -p spec && tar xvf cpu2006.tar --directory=spec
# if you're installing spec somewhere else, you need to change our config's ROOTDIR variable too.
cp build_spec_utils.sh spec
cp spectools.patch spec
cp llvm-aarch64-mte-tagcfi.cfg spec/config/
cd spec

# install spec dependencies if you don't use `apt` pkg manager
# Nix users: nix develop
./build_spec_utils.sh
source shrc # `runspec` binary should be accessible after
```

## Compiling and Running SPEC

```sh
# building programs,
runspec --config=llvm-aarch64-mte-tagcfi    \ # our config file
        -e default-static                   \ # names of target extension: ral-static, vtt-static, ifct+vtt-static, ...
        --action build                      \ # see runspec doc
        int                                 \ # int or all_cpp
        --make_bundle ral-static.int          # bundle filename, --unpack_bundle <bundlename> to unpack

# Running the bench,
runspec --config=llvm-aarch64-mte-tagcfi        \
        -e default-static                       \
        --tune base --size test --iteration 5   \ # runspec doc
        -S core=cxc                             \ # little, big or cxc to fix the core. OR `-S ON_VM` to run on qemu
        int --noreportable

# NOTE: runspec will run the binaries via qemu-aarch64 if you use `-S ON_VM` flag
# Don't forget to install qemu
# NOTE: `-S core={little,big,cxc}` needs `bench_runner` script on PATH, you can find it under this repo.

# on x86 machines
runspec --config=llvm-aarch64-mte-tagcfi -e ral-static --tune base --size test --iteration 1 -S ON_VM --noreportable all_cpp --make_bundle vtt-static.all_cpp

# on pixel8
runspec --config=llvm-aarch64-mte-tagcfi -e ral-static --tune base --size test --iteration 1 -S core=cxc --noreportable all_cpp --make_bundle vtt-static.all_cpp
```

## Script: `bench_runner`
Usage : `bench_runner {big|little|cxc} <command>`
e.g.,
```sh
$bench_runner cxc echo "hello"
```

### Porting `bench_runner` to different machine
```sh
# open up the file
cores=(0 4 8)               # change CPU number, e.g., pixel9 has (0, 4, 7)
fixed_freq=(1328 1836 2363) # Fixed freq during the execution
orig_min=(324 402 500)      # Default minimum frequency
orig_max=(1704 2367 2850)   # Default maximum frequency
```

## How to plot

```sh
python3 plot_all.py
```

### How to add results

```python3
# add this code to main of `plot_all.py`
# `runspec` assigns a number whenever you run the benchmark, e.g., runcode is 96 from CPU2006.096.log
dicts = (PlotGroup("<plot_title>", "<dir/path/to/spec/log/file>")
        .add_group([baseline_runcode, tuned_runcode], "label1"))
dicts.plot()
```

## Fixed Frequency

```
host$ adb -s <serial> push lockClocks.sh /data/local/tmp/lockClocks.sh
host$ adb -s <serial> shell chmod +x /data/local/tmp/lockClocks.sh
host$ adb -s <serial> shell /data/local/tmp/lockClocks.sh

# pixel 8 Pro (husky)
Locked CPU 0 to 1425000 / 1704000 KHz
Locked CPU 1 to 1425000 / 1704000 KHz
Locked CPU 2 to 1425000 / 1704000 KHz
Locked CPU 3 to 1425000 / 1704000 KHz
Locked CPU 4 to 1945000 / 2367000 KHz
Locked CPU 5 to 1945000 / 2367000 KHz
Locked CPU 6 to 1945000 / 2367000 KHz
Locked CPU 7 to 1945000 / 2367000 KHz
Locked CPU 8 to 2363000 / 2914000 KHz

# pixel 9 Pro
Locked CPU 0 to 1696000 / 1950000 KHz
Locked CPU 1 to 1696000 / 1950000 KHz
Locked CPU 2 to 1696000 / 1950000 KHz
Locked CPU 3 to 1696000 / 1950000 KHz
Locked CPU 4 to 2130000 / 2600000 KHz
Locked CPU 5 to 2130000 / 2600000 KHz
Locked CPU 6 to 2130000 / 2600000 KHz
Locked CPU 7 to 2499000 / 3105000 KHz

# ampere one
analyzing CPU 50:
  driver: cppc_cpufreq
  CPUs which run at the same hardware frequency: 50
  CPUs which need to have their frequency coordinated by software: 50
  maximum transition latency:  Cannot determine or is not supported.
  hardware limits: 1000 MHz - 3.20 GHz
  available cpufreq governors: conservative ondemand userspace powersave performance schedutil
  current policy: frequency should be within 2.60 GHz and 2.60 GHz.
                  The governor "performance" may decide which speed to use
                  within this range.
  current CPU frequency: 2.55 GHz (asserted by call to hardware)
```
