#!/bin/bash

ndk-build
./push.sh

adb shell killall testnapp

adb shell /data/local/tmp/testnapp &
adb shell /data/local/tmp/testnapp &

pids=$(adb shell ps -e | grep testnapp | sed 's/[ ][ ]*/ /g' | cut -d' ' -f2)
echo "pid is " $pids
adb shell /data/local/tmp/setcmp $pids
adb shell killall testnapp

