#!/bin/bash

# registers a firmware, list the firmwares folder and check it contains it

if [ -n "${V+x}" ]
then
	set -xu
fi

set -e

on_exit() {
	# we don't want to fail here, to guarantee the cleanup
	set +e
	rm example_firmware.ext2
	if [ -n "${firmware}" ]; then
		fdc drop firmwares ${firmware}
	fi
}

TESTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

tar xf ${TESTS_DIR}/../examples/example_firmware.tar.bz2

trap on_exit EXIT

fdc prepare firmwares ${PWD}/example_firmware.ext2

answer=$(fdc list firmwares)
firmware=${answer%[*}
pattern='[a-z_]+\[[a-f0-9]+\]'
[[ ${answer} =~ ${pattern} ]]