#!/usr/bin/env bash
adb shell setprop arm64.memtag.bootctl memtag
#setprop persist.arm64.memtag.default sync
#setprop persist.arm64.memtag.app_default sync
adb shell reboot