#!/bin/bash

# kill firmwared and check if it's still alive

if [ -n "${VV+x}" ]
then
	set -x
fi

set -eu

answer=$(fdc quit)
expected="firmwared said bye bye"
[ "${answer}" = "${expected}" ]

# 1 tenth of second should be sufficient for firmwared to quit
sleep .1

# check if firmwared is still alive
if pidof firmwared; then
	exit 1
else
	exit 0
fi