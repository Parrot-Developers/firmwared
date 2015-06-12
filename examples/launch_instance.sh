#!/bin/bash

if [ "$1" = "--help" ] || [ "$1" = "-h" ] || [ "$1" = "-?" ];
then
	echo "Ask firmwared to prepare and launch a firmware instance."
	echo "If the \"firmware\" environment variable is set, it will be used "
	echo "as the firmware, otherwise, the first firmware found in the "
	echo "firmwared firmwares repository will be used."
	exit
fi

set -x

. out/sphinx-base/staging/native-wrapper.sh

# defaults to the first firmware found
firmware=${firmware:-$(fdc list firmwares | sed 's/\[.*//g')}

# create an instance and store it's name for future use
instance=$(fdc prepare ${firmware} | tail -n 1 | sed 's/.*: //g');

fdc show instances $instance
# set which process will be used as the pid 1
fdc set_property instances $instance cmdline[0] /sbin/boxinit
# retrieve the pts which will be used by the boxinit console service
console_pts=$(fdc get_property instances $instance inner_pts)
# strip the leading /dev
console_pts=${console_pts#/dev}
# pass it as an argument to boxinit
fdc set_property instances $instance cmdline[1] ro.boot.console=${console_pts}
# set the ro.hardware, boxinit will use it to load the corresponding .rc
fdc set_property instances $instance cmdline[2] ro.hardware=mk3_sim_pc
# the first nil command-line argument ends the array, it is needed to get rid of
# the parameters which were already registered in the command-line
fdc set_property instances $instance cmdline[3] nil

# check everything went well
fdc show instances $instance

# watch what's going on the "simulated uart" console
x-terminal-emulator -e "screen $(fdc get_property instances $instance outer_pts)" &

# libpomp doesn't like multiple parallel connections
sleep .1

# launch the instance
fdc start $instance

# wait for user input
read -p "Press enter to end the session ..."

# cleanup
fdc kill $instance
fdc drop instances $instance
