#!/bin/bash

# test case to reproduce a segfault of firmwared with the show command

set -x

mkdir /tmp/plop
firmware=( $(fdc prepare firmwares /tmp/plop) )
firmware=${firmware[$((${#firmware[@]} - 1))]}

# here, firmwared used to crash, because the product property couldn't be
# populated, due to the lack of the build.prop file
fdc show firmwares $firmware

fdc drop firmwares $firmware