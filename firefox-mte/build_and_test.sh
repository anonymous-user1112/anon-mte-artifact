# android builds
./mach build && MOZCONFIG=mozconfig_android_copy ./mach build && MOZCONFIG=mozconfig_android_mte ./mach build

# install & test stock
./mach install && sleep 5 &&  ~/Android/Sdk/platform-tools/adb logcat -c && ~/Android/Sdk/platform-tools/adb shell am start -n org.mozilla.geckoview_example/.GeckoViewActivity --es env0 MOZ_LOG="AndroidTestLog:5" MOZ_DISABLE_CONTENT_SANDBOX=1 MOZ_DISABLE_RDD_SANDBOX=1 MOZ_DISABLE_SOCKET_PROCESS_SANDBOX=1 MOZ_DISABLE_GPU_SANDBOX=1 MOZ_DISABLE_GMP_SANDBOX=1 MOZ_DISABLE_VR_SANDBOX=1 MOZ_DISABLE_UTILITY_SANDBOX=1 && ~/Android/Sdk/platform-tools/adb logcat Gecko:V '*:S' | stdbuf -o0 grep AndroidTestLog | tee ../benchmarks/ff-expat-android/console_stock.log

# install & test copy
MOZCONFIG=mozconfig_android_copy  ./mach install && sleep 5 &&  ~/Android/Sdk/platform-tools/adb logcat -c && ~/Android/Sdk/platform-tools/adb shell am start -n org.mozilla.geckoview_example/.GeckoViewActivity --es env0 MOZ_LOG="AndroidTestLog:5" MOZ_DISABLE_CONTENT_SANDBOX=1 MOZ_DISABLE_RDD_SANDBOX=1 MOZ_DISABLE_SOCKET_PROCESS_SANDBOX=1 MOZ_DISABLE_GPU_SANDBOX=1 MOZ_DISABLE_GMP_SANDBOX=1 MOZ_DISABLE_VR_SANDBOX=1 MOZ_DISABLE_UTILITY_SANDBOX=1 && ~/Android/Sdk/platform-tools/adb logcat Gecko:V '*:S' | stdbuf -o0 grep AndroidTestLog | tee ../benchmarks/ff-expat-android/console_copy.log

# install & test mte
MOZCONFIG=mozconfig_android_mte ./mach install && sleep 5 &&  ~/Android/Sdk/platform-tools/adb logcat -c && ~/Android/Sdk/platform-tools/adb shell am start -n org.mozilla.geckoview_example/.GeckoViewActivity --es env0 MOZ_LOG="AndroidTestLog:5" MOZ_DISABLE_CONTENT_SANDBOX=1 MOZ_DISABLE_RDD_SANDBOX=1 MOZ_DISABLE_SOCKET_PROCESS_SANDBOX=1 MOZ_DISABLE_GPU_SANDBOX=1 MOZ_DISABLE_GMP_SANDBOX=1 MOZ_DISABLE_VR_SANDBOX=1 MOZ_DISABLE_UTILITY_SANDBOX=1 && ~/Android/Sdk/platform-tools/adb logcat Gecko:V '*:S' | stdbuf -o0 grep AndroidTestLog | tee ../benchmarks/ff-expat-android/console_mte.log