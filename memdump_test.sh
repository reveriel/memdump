#!/bin/bash

ndk-build
./push.sh
adb shell killall testnapp
adb shell /data/local/tmp/testnapp &
pid=$(adb shell ps -e | grep testnapp | sed 's/[ ][ ]*/ /g' | cut -d' ' -f2 | head -n1)
echo "pid is " $pid
adb shell /data/local/tmp/memdump $pid
adb shell killall testnapp

