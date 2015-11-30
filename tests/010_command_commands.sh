#!/bin/bash

# asks for the list of registered firmwared commands and check it

if [ -n "${V+x}" ]
then
	set -x
fi

set -eu

answer=$(fdc commands)
expected="START SET_PROPERTY LIST PROPERTIES GET_PROPERTY SHOW KILL FOLDERS COMMANDS DROP REMOUNT GET_CONFIG CONFIG_KEYS PING HELP QUIT VERSION PREPARE RESTART"
[ "${answer}" = "${expected}" ]