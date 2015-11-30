#!/bin/bash

# ask for the help of the "help" command and check the answer's content

if [ -n "${V+x}" ]
then
	set -xu
fi

set -e

answer=$(fdc help help)
expected="Command HELP
Synopsis: HELP COMMAND
Overview: Sends back a little help on the command COMMAND."
[ "${answer}" = "${expected}" ]