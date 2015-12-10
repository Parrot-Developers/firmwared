#!/bin/bash

# prepares a firmware and check the output of the show command looks ok

if [ -n "${V+x}" ]
then
	set -x
fi

set -eu

on_exit() {
	status=$?
	# we don't want to fail here, to guarantee the cleanup
	set +e
	rm -f example_firmware.ext2
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

pattern='[a-z0-9_]+:.*'
fdc show firmwares ${firmware} | while read line
do
	[[ ${line} =~ ${pattern} ]]
done