#!/bin/bash

# asks for the list of registered firmwared commands and check it

if [ -n "${V+x}" ]
then
	set -x
fi

set -eu

answer=$(fdc properties firmwares)
expected="name sha1 path product hardware"
[ "${answer}" = "${expected}" ]