#!/bin/bash

# prepares an instance and check the return of the fdc kill instance

if [ -n "${VV+x}" ]
then
	set -xu
fi

set -e

on_exit() {
	status=$?
	# we don't want to fail here, to guarantee the cleanup
	set +e
	rm example_firmware.ext2
	if [ ${status} -eq 0 ]; then
		fdc kill ${instance}
	fi
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

instance=$(fdc list instances)
instance=${instance%[*}

answer=$(fdc start ${instance})
expected="${instance} started"
[ "${answer}" = "${expected}" ]