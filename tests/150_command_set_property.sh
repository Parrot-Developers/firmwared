#!/bin/bash

# prepares an instance, sets the "interface" property and check it worked

if [ -n "${VV+x}" ]
then
	set -x
fi

set -eu

on_exit() {
	status=$?
	# we don't want to fail here, to guarantee the cleanup
	set +e
	rm example_firmware.ext2
	if [ -n "${instance}" ]; then
		fdc drop instances ${instance}
	fi
	if [ -n "${firmware}" ]; then
		fdc drop firmwares ${firmware}
	fi
	exit ${status}
}

TESTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

tar xf ${TESTS_DIR}/../examples/example_firmware.tar.bz2

trap on_exit EXIT

fdc prepare firmwares ${PWD}/example_firmware.ext2

firmware=$(fdc list firmwares)
firmware=${firmware%[*}

fdc prepare instances ${firmware}
echo ${firmware}
instance=$(fdc list instances)
instance=${instance%[*}
echo ${instance}
fdc set_property instances ${instance} interface plop
answer=$(fdc get_property instances ${instance} interface)
expected="plop"
[ "${answer}" = "${expected}" ]