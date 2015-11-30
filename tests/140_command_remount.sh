#!/bin/bash

# prepares an instance, issues an fdc remount and check what the command says

if [ -n "${V+x}" ]
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
answer=$(fdc remount ${instance})
expected="${instance} remounted"
[ "${answer}" = "${expected}" ]