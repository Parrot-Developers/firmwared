#!/bin/bash

# prepares a firmware and check it's "product" property equals "dragonfly"

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
	exit ${status}
}

TESTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

tar xf ${TESTS_DIR}/../examples/example_firmware.tar.bz2

trap on_exit EXIT

fdc prepare firmwares ${PWD}/example_firmware.ext2

firmware=$(fdc list firmwares)
firmware=${firmware%[*}

answer=$(fdc get_property firmwares ${firmware} sha1)
expected=2915c77028cccee9b2820e4c9df06a8a413b0252
[ "${answer}" = "${expected}" ]