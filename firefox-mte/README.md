# Modification of Firefox to test BufLock.

## Running on a Desktop environments (debian/ubuntu on aarch64 assumed)

To run the test on desktop computers, start by building the required Firefox
builds.

Start with the one time configuration of your system

```bash
./mach --no-interactive bootstrap --application-choice "Firefox for Desktop"
```

Build the three firefox configurations - stock (no protections), copy, mte (buflock)

```bash
MOZCONFIG=./mozconfig_stock ./mach build
MOZCONFIG=./mozconfig_copy ./mach build
MOZCONFIG=./mozconfig_mte ./mach build
```

You'll now have three output folders
- firefox_release_stock
- firefox_release_copy
- firefox_release_mte


Then, setup the benchmark environment.

- Pick the CPU numbers you want to run on, e.g. 1 and 2
- Boot the OS with CPU 2 isolated by passing `--isolcpus=1,2` during boot up.
- Disable hyperthreading
    ```bash
    sudo bash -c "echo off > /sys/devices/system/cpu/smt/control"
    ```
- Disable frequency scaling and set the performance governor
    ```bash
    sudo cpufreq-set -c 1 -g performance
    sudo cpufreq-set -c 1 --min 2200MHz --max 2200MHz

    sudo cpufreq-set -c 2 -g performance
    sudo cpufreq-set -c 2 --min 2200MHz --max 2200MHz
    ```
- If you are running the benchmark in a remote computer without a GUI, you will
need to install and run `Xvfb` also, to simulate the presence of a UI.

    ```bash
    # Note the different capitalizations
    sudo apt install xvfb
    Xvfb :99 &
    export DISPLAY=:99
    ```

Then, use the `testsRunBenchmark` script. This script expects the following
arguments

```bash
.\testsRunBenchmark <output_folder> <test_name> <space_separated_build_names> <space_separated_cpus_to_pin_to>
```

- The first argument is the output folder. The output folder should not already exist.
- The second argument is the test-name. This should be set to `expat_perf_test`.
- The third argument is strategy for Firefox buffer checks: `stock` `copy` `mte`. To run multiple builds, you can specify multiple options as a space separated string `'stock copy mte'`.
- The fourth argument is the cpu numbers you want to pin the expat rendering to. To run multiple times on different CPUs, you can specify multiple CPU options as a space separated string `'1 2'`


So for example, you can run something like this

```
./testsRunBenchmark /some/existing/output/path/new_folder_name expat_perf_test 'stock copy' '1 2'
```

The script will generate the folders
`/some/existing/output/path/new_folder_name/CPU1` and
`/some/existing/output/path/new_folder_name/CPU2` in the above example. Each
folder will have all the raw output, analyzed data, and a summary in the file
`compare_stock_terminal_analysis.json.dat`

## Running on Android environment

Unfortunately, running the build and tests on android are a bit manual as Firefox doesn't have the infrastructure to easily automate this.

To run the test on android phones, start by building the required Firefox
builds.

Start with the one time configuration of your system

```bash
./mach --no-interactive bootstrap --application-choice GeckoView/Firefox for Android
```

Build the three firefox configurations - stock (no protections), copy, mte (buflock)
**Note**: there may be a transient error in the build system that I haven't fixed.
If you get an error, try rerunning the build command to see if the error goes away.

```bash
MOZCONFIG=./mozconfig_android_stock ./mach build && MOZCONFIG=./mozconfig_android_stock ./mach package
MOZCONFIG=./mozconfig_android_copy ./mach build && MOZCONFIG=./mozconfig_android_copy ./mach package
MOZCONFIG=./mozconfig_android_mte ./mach build && MOZCONFIG=./mozconfig_android_mte ./mach package
```

You'll now have three output folders
- firefox_release_android_stock
- firefox_release_android_copy
- firefox_release_android_mte

Next, add adb to path if not present
export PATH=$PATH:~/Android/Sdk/platform-tools/

Setup the benchmarking environment
- Push the lockClocks scripts from `https://github.com/XXXXXXXX/mte-playground/blob/main/lockClocks.sh` and then run it to set the CPUs to 80% of their stock frequency
    ```bash
    adb push lockClocks.sh /data/local/tmp/lockClocks.sh
    adb shell chmod +x /data/local/tmp/lockClocks.sh
    adb shell /data/local/tmp/lockClocks.sh
    ```
- Copy test to Android
    ```bash
    adb push testing/talos/talos/tests/expat_perf_test /tmp/
    ```


Now you can run the test by running the commands below with the three Firefox
configurations --- `mozconfig_android_stock`, `mozconfig_android_copy`,
`mozconfig_android_mte`

    ```bash
    # Install the chosen Firefox with
    MOZCONFIG=mozconfig_android_<pick_from_above> ./mach install

    # For MTE runs, you must enable MTE and reboot to apply the setting.
    # Our code automatically sets its mode.
    adb shell setprop arm64.memtag.bootctl memtag
    adb shell setprop persist.arm64.memtag.default off      # default MTE mode for native executables
    adb shell setprop persist.arm64.memtag.app_default off  # defualt MTE mode for apps
    adb reboot # apply

    # Clear the adb log
    adb logcat -c

    # Start Gecko and filter adb log to Firefox results. The command to gecko specifies
    # 1. logging of benchmark-test results and specifying
    # 2. The CPU core to pin to. IMPORTANT: Choose the core you want to test
    # These should be one of
    #   Pixel 8/9 Little Core: 0
    #   Pixel 8/9 Big Core: 4
    #   Pixel 8 XCore: 8
    #   Pixel 9 XCore: 7
    ### this is a test command to see if core pinning works as expected.
    adb shell am start -n org.mozilla.geckoview_example/.GeckoViewActivity --es env0 MOZ_LOG=MTETestLog:5 --es env1 EXPAT_PIN_CORE=7  && adb logcat Gecko:V '*:S'

    ### Run expat benchmark with this command (make sure `/tmp/expat_perf_test` directory exists)
    ### Tip 1: make sure the screen light is always on during the benchmark. light off stops the benchmark.
    ### Tip 2: Pixel 9 only allows pinning to the little cores when the screen is off, so type the command after turning on the light.
    adb shell am start -n org.mozilla.geckoview_example/.GeckoViewActivity -d "file:///tmp/expat_perf_test/expattest.html" --es env0 MOZ_LOG=MTETestLog:5 --es env1 EXPAT_PIN_CORE=7  && adb logcat Gecko:V '*:S'

    ### Or, you can manually run the benchamrk
    # Navigate the browser to the URL "file:///tmp/expat_perf_test/expattest.html" and wait for the test to finish

    # Confirm that the configuration you are running is the one you expect.
    # You will see as part of the log, the line
    # <EXPATLOGPREFIX> sched_setaffinity on core : <EXPAT_PIN_CORE>
    # EXPATLOGPREFIX will be one of "STOCK", "COPY", "MTE"
    #
    # For the MTE configuration above you will also see the following log lines where MTE is being enabled
    # "MTE Trying to enable MTE"
    # "MTE Done

    # You will see output being logged in the adb console of the form
    # <EXPATLOGPREFIX> Capture_Time:Expat,<BenchmarkIteration>,<BenchmarkTime>,<PID>
    # <EXPATLOGPREFIX> Capture_Time:Expat,1,123456,987
    # <EXPATLOGPREFIX> Capture_Time:Expat,2,123444,987
    # ...
    # There are 110 benchmark iterations (specified in testing/talos/talos/tests/expat_perf_test/expattest.html)
    # So wait till you see:
    # # <EXPATLOGPREFIX> Capture_Time:Expat,110,...

    ###################

    # Terminate the adb log command that is running with Ctrl+C

    # Stop gecko
    adb shell am force-stop org.mozilla.geckoview_example
    ```

Now you can collect and analyze the performance data

```bash
export OUTPUT_FOLDER=../benchmarks/ff-expat-android$(date --iso=seconds)/
# Pick the appropriate option
export OUTPUT_FILENAME=$OUTPUT_FOLDER/xcore/stock_terminal_output
export OUTPUT_FILENAME=$OUTPUT_FOLDER/big/stock_terminal_output
export OUTPUT_FILENAME=$OUTPUT_FOLDER/little/stock_terminal_output
export OUTPUT_FILENAME=$OUTPUT_FOLDER/xcore/copy_terminal_output
export OUTPUT_FILENAME=$OUTPUT_FOLDER/big/copy_terminal_output
export OUTPUT_FILENAME=$OUTPUT_FOLDER/little/copy_terminal_output
export OUTPUT_FILENAME=$OUTPUT_FOLDER/xcore/mte_terminal_output
export OUTPUT_FILENAME=$OUTPUT_FOLDER/big/mte_terminal_output
export OUTPUT_FILENAME=$OUTPUT_FOLDER/little/mte_terminal_output

# Manually copy the output from the adb log command on the screen to a file $OUTPUT_FILENAME.log to your host computer

# Extract the stats from the console
./testsExtractFromLogs.py $OUTPUT_FILENAME.log >> $OUTPUT_FILENAME.json
```

Repeat the above for all the builds.

Finally, you can compare the data

```bash
# Compare the stats from different builds
./testsAnalyzeExtractedLogs.py $OUTPUT_FOLDER/xcore
./testsAnalyzeExtractedLogs.py $OUTPUT_FOLDER/big
./testsAnalyzeExtractedLogs.py $OUTPUT_FOLDER/little
```

You will see three files with the final data

- `$OUTPUT_FOLDER/xcore/compare_stock_terminal_analysis.json.dat`
- `$OUTPUT_FOLDER/big/compare_stock_terminal_analysis.json.dat`
- `$OUTPUT_FOLDER/little/compare_stock_terminal_analysis.json.dat`
