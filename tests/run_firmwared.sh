#!/bin/bash
# restarts firmwared until the stop_firmwared_tests file is created
# must be ran as root

set -x

mkdir -p mount firmwares

while ! [ -e stop_firmwared_tests ]; do
	firmwared firmwared.conf
	sleep 1
done
rm -rf stop_firmwared_tests mount firmwares
