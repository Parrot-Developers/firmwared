#!/bin/bash

# asks for the list of registered firmwared commands and check it

if [ -n "${VV+x}" ]
then
	set -x
fi

set -eu

answer="$(echo $(printf "%s\n" $(fdc commands) | sort))"
expected="ADD_PROPERTY COMMANDS CONFIG_KEYS DROP FOLDERS GET_CONFIG GET_PROPERTY HELP KILL LIST PING PREPARE PROPERTIES QUIT REMOUNT RESTART SET_PROPERTY SHOW START VERSION"
test "${answer}" = "${expected}"
