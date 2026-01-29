#!/bin/sh
module=soil-driver-i2c
device=soil-driver-i2c
cd `dirname $0`
# invoke rmmod with all arguments we got
rmmod $module || exit 1

# Remove stale nodes

rm -f /dev/${device}