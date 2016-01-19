#!/bin/bash

# asks for the list of registered firmwared commands and check it

if [ -n "${VV+x}" ]
then
	set -x
fi

set -eu

answer=$(fdc properties firmwares)
expected="name sha1 base_workspace path uuid"
[ "${answer}" = "${expected}" ]