# Automatic tests for firmwared

## Overview

These tests are used to test the high level operations on firmwared.

## Prerequisites

Move to the root of the workspace, where you built firmwared, then:

        cp packages/firmwared/examples/firmwared.conf .
        sed -i "s#base_dir = .*#base_dir = \"$(pwd)/\"#g" firmwared.conf
        . Alchemy-out/linux-native-x64/final/native-wrapper.sh

## Running one test

Firmwared must be running and with no firmware and no instance prepared.
It must be launched as root, in a while loop, restarting it after 1 second, like
that :

        while [ 1 ]; do FIRMWARED_VERBOSE_HOOK_SCRIPTS=y ULOG_LEVEL=D firmwared firmwared.conf ; sleep 1; done

Then individual tests can be launched, as a normal user, like that:

        ./packages/firmwared/tests/070_command_help.sh

The return status indicates the success or failure of the test.

## Run all the tests

Firmwared _must not_ be running, plus the directories **mount** and
**firmwares** will be **removed**.

        sudo PATH=$PATH LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
                ./packages/firmwared/tests/run_firmwared.sh
        <Ctrl+z>
        bg
        ./packages/firmwared/tests/run_all.sh

