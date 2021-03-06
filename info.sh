#!/bin/bash

. config_apps.sh

ndk-build
./push.sh

for app in $apps
do
    # echo $app

    # #vma
    pid=$(adb shell ps -e | grep -e $app$ | sed 's/[\t ][\t ]*/ /g' | cut -d' ' -f2)
    # echo "number of vma:" $(adb shell cat /proc/$pid/maps | wc)
    if [[ pid -eq 0 ]]; then
        echo "pid = 0"
        echo "app = $app"
        exit -1
    fi
    adb shell /data/local/tmp/memdump $pid -m

    # proc size
    # adb shell top -p $pid -bn1 | tail -n1

done
