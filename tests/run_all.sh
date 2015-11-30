#!/bin/bash

# run all the scripts in the tests folder, whose name follows the pattern
# +([0-9])_*.sh
# returns non-zero iif at least one test failed
# some assumptions are made :
#  * the firmwares repository is empty
#  * the container_interface property equals "eth0"
#  * firmwared will be restarted at least 20ms after it quits

if [ -n "${V+x}" ]
then
	set -x
fi

set -eu

exit_status=0

TESTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

on_exit() {
	local result=$?
	if [ ${result} -ne 0 ]; then
		echo "                 -> failed with status $? *******************"
		exit 1
	fi
}

trap on_exit EXIT

# enable extended pattern support required for the following +(...) pattern
shopt -s extglob
for t in ${TESTS_DIR}/+([0-9])_*.sh; do
	echo "******************* RUNNING TEST $t *******************"
	if $t; then
		echo -e "                 -> \033[1;32mOk\033[0m *******************"
	else
		echo -e "                 -> \033[7;31mFail\033[0m *******************"
		exit_status=1
	fi
done

if [ ${exit_status} -eq 0 ]; then
	echo All tests Ok
else
	echo At least one test failed
fi

exit ${exit_status}