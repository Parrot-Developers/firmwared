#!/bin/bash

# asks for the list of registered firmwared folders and check it

if [ -n "${VV+x}" ]
then
	set -x
fi

set -eu

answer=$(fdc folders)
expected="instances firmwares"
[ "${answer}" = "${expected}" ]