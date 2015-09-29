#!/bin/bash

if [ "$1" = "--help" ] || [ "$1" = "-h" ] || [ "$1" = "-?" ];
then
	cat <<EndOfUsage
Asks firmwared to prepare and launch a firmware instance.
If the "firmware" environment variable is set, it will be used as the firmware,
otherwise, the first firmware found in the firmwared registered firmwares list
will be used.

usage: ${0/*\//} [wifi_config [eth0_name]]
	Where wifi_config has the form:
		stolen_interface:target_name:cidr_ip_address
	example : ${0/*\//} wlan0:eth0:192.168.0.1/24
	eth0_name is the name the instance's veth's end will have (defaults to
	eth0)
EndOfUsage
	exit
fi

wifi_config=${1:-}
eth0_name=${2:-}

if [ -n "${V+x}" ]
then
	set -x
fi

set -e

. out/sphinx-base/staging/native-wrapper.sh

# defaults to the first firmware found
firmware=${firmware:-$(fdc list firmwares | sed 's/\[.*//g')}

if [ -z "${firmware}" ]; then
	echo "No suitable firmware found, please prepare at least one or set"
	echo "the firmware environment variable to the final directory you "
	echo "want to use as a firmware."
	exit 1
fi

drop_firmware="false"
# check if the firmware is a registered one, otherwise, consider it as an
# argument for an 'fdc prepare firmware' command
if [ -z "$(fdc list firmwares | grep ${firmware})" ]; then
	drop_firmware="true"
	firmware=$(fdc prepare firmwares ${firmware})
	if [ -n "$(echo ${firmware} | grep "File exists")" ]; then
		echo "Firmware already registered."
		echo -n "If it's normal, just retry with the "
		echo "\"firmware\" environment variable set to an id of this "
		echo "firmware."
		exit 1
	fi
	firmware=$(echo ${firmware} | tail -n 1 | sed 's/.*: //g')
fi

if [ -z "${firmware}" ]; then
	echo "something went wrong, no firmware defined, aborting"
	exit 1
fi

# create an instance and store it's name for future use
instance=$(fdc prepare instances ${firmware} | grep 'name:' | sed 's/.*: //g');

fdc show instances $instance
# set which process will be used as the pid 1
fdc set_property instances $instance cmdline[0] /sbin/boxinit
# retrieve the pts which will be used by the boxinit console service
console_pts=$(fdc get_property instances $instance inner_pts)
# strip the leading /dev
console_pts=${console_pts#/dev}
# pass it as an argument to boxinit
fdc set_property instances $instance cmdline[1] ro.boot.console=${console_pts}
# set the ro.hardware property, retrieved from the firmware's properties
hardware=$(fdc get_property firmwares $firmware hardware)
fdc set_property instances $instance cmdline[2] ro.hardware=${hardware}
# the first nil command-line argument ends the array, it is needed to get rid of
# the parameters which were already registered in the command-line
fdc set_property instances $instance cmdline[3] nil

if [ -n "${wifi_config}" ]; then
	fdc set_property instances $instance stolen_interface ${wifi_config}
	if [ -n "${eth0_name}" ]; then
		fdc set_property instances $instance interface ${eth0_name}
	fi
fi

# check everything went well
fdc show instances $instance

# watch what's going on the "simulated uart" console
outer_pts=$(fdc get_property instances $instance outer_pts)
x-terminal-emulator -e "parrotcom ${outer_pts}" &

# if the terminal doesn't have the time to launch before the instance starts,
# some outputs can be garbage
sleep .5

# launch the instance
fdc start $instance

# connect adb
id=$(fdc get_property instances ${instance} id)
net_first_two_bytes=$(fdc get_config net_first_two_bytes)
adb connect ${net_first_two_bytes}${id}.1:9050

# wait for user input
read -p "Press enter to end the session ..."

# cleanup
fdc kill $instance
fdc drop instances $instance
if [ "${drop_firmware}" = "true" ]; then
	fdc drop firmwares ${firmware}
fi
