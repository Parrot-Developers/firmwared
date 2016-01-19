#!/bin/bash

# sends a ping and expects a pong

if [ -n "${VV+x}" ]
then
	set -xu
fi

set -e

answer=$(fdc ping)
expected="PONG"
[ "${answer}" = "${expected}" ]