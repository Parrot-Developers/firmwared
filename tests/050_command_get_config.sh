#!/bin/bash

# sends a fdc get_config container_interface command and expects the reply to be
# eth0. Of course, this will work iif the user hasn't changed this config key...

if [ -n "${V+x}" ]
then
	set -x
fi

set -eu

answer=$(fdc get_config container_interface)
expected="eth0"
[ "${answer}" = "${expected}" ]