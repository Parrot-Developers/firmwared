#!/bin/bash

# ask for the help of the "help" command and check the answer's content

if [ -n "${V+x}" ]
then
	set -xu
fi

set -e

answer=$(fdc help folders)
expected="Command FOLDERS
Synopsis: FOLDERS
Overview: Asks the server to list the currently registered folders."
[ "${answer}" = "${expected}" ]