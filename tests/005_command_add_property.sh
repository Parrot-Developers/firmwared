#!/bin/bash

# registers a firmware, add and set a custom property and test if all went right

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
	if [ -n "${firmware}" ]; then
		fdc drop firmwares ${firmware}
	fi
	# there is no other way to remove the foo property
	fdc quit
	sleep 1.5
	exit ${status}
}

TESTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

tar xf ${TESTS_DIR}/../examples/example_firmware.tar.bz2

trap on_exit EXIT

fdc prepare firmwares ${PWD}/example_firmware.ext2

firmware=$(fdc list firmwares)
firmware=${firmware%[*}

fdc add_property firmwares foo
fdc set_property firmwares ${firmware} foo bar

answer=$(fdc get_property firmwares ${firmware} foo)
expected="bar"
[ "${answer}" = "${expected}" ]
