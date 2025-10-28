#!/bin/sh
module=soil-driver
device=soil-driver
mode="666"
cd `dirname $0`
set -e
# Group: since distributions do it differently, look for wheel or use staff
if grep -q '^staff:' /etc/group; then
    group="staff"
else
    group="wheel"
fi

if [ -e ${module}.ko ]; then
    echo "Loading local built file ${module}.ko"
    insmod ./$module.ko $* || exit 1
else
    echo "Local file ${module}.ko not found, attempting to modprobe"
    modprobe ${module} || exit 1
fi
