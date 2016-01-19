#!/bin/bash

# asks for the list of registered firmwared configuration keys and check it

if [ -n "${VV+x}" ]
then
	set -x
fi

set -eu

answer=$(fdc config_keys)
expected="apparmor_profile container_interface curl_hook disable_apparmor dump_profile host_interface_prefix mount_hook mount_path net_first_two_bytes net_hook post_prepare_instance_hook prevent_removal resources_dir repository_path socket_path verbose_hook_scripts"
[ "${answer}" = "${expected}" ]