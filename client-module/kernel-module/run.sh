#!/bin/bash

adb shell rmmod test.ko
adb shell dmesg -c
adb push test.ko /data/local/tmp/test.ko
adb shell insmod /data/local/tmp/test.ko
adb shell dmesg
