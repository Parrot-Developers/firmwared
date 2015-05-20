/**
 * @file config.c
 * @brief
 *
 * @date Apr 22, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <stdlib.h>

#define ULOG_TAG firmwared_config
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_config);

#include "config.h"

#ifndef MOUNT_HOOK_DEFAULT
#define MOUNT_HOOK_DEFAULT "/usr/libexec/firmwared/mount.hook"
#endif /* MOUNT_HOOK_DEFAULT */

#ifndef SOCKET_PATH_DEFAULT
#define SOCKET_PATH_DEFAULT "/var/run/firmwared.sock"
#endif /* SOCKET_PATH_DEFAULT */

#ifndef FOLDERS_RESOURCES_DIR_DEFAULT
#define FOLDERS_RESOURCES_DIR_DEFAULT "/usr/share/firmwared/"
#endif /* FOLDERS_RESOURCES_DIR_DEFAULT */

#ifndef FIRMWARE_REPOSITORY_DEFAULT
#define FIRMWARE_REPOSITORY_DEFAULT "/usr/share/firmwared/firmwares/"
#endif /* FIRMWARE_REPOSITORY_DEFAULT */

#ifndef INSTANCES_MOUNT_PATH_DEFAULT
#define INSTANCES_MOUNT_PATH_DEFAULT "/var/run/firmwared/mount_points"
#endif /* INSTANCES_MOUNT_PATH_DEFAULT */

#ifndef NET_HOOK_DEFAULT
#define NET_HOOK_DEFAULT "/usr/libexec/firmwared/net.hook"
#endif /* NET_HOOK_DEFAULT */

struct config {
	char *env;
	char *value;
	char *default_value;
};

static struct config configs[CONFIG_NB] = {
		[CONFIG_MOUNT_HOOK] = {
				.env = "FIRMWARED_MOUNT_HOOK",
				.default_value = MOUNT_HOOK_DEFAULT,
		},
		[CONFIG_SOCKET_PATH] = {
				.env = "FIRMWARED_SOCKET_PATH",
				.default_value = SOCKET_PATH_DEFAULT,
		},
		[CONFIG_RESOURCES_DIR] = {
				.env = "FIRMWARED_RESOURCES_DIR",
				.default_value = FOLDERS_RESOURCES_DIR_DEFAULT,
		},
		[CONFIG_FIRMWARE_REPOSITORY] = {
				.env = "FIRMWARED_REPOSITORY_PATH",
				.default_value = FIRMWARE_REPOSITORY_DEFAULT,
		},
		[CONFIG_BASE_MOUNT_PATH] = {
				.env = "FIRMWARED_MOUNT_PATH",
				.default_value = INSTANCES_MOUNT_PATH_DEFAULT,
		},
		[CONFIG_NET_HOOK] = {
				.env = "FIRMWARED_NET_HOOK",
				.default_value = NET_HOOK_DEFAULT,
		},
};

__attribute__((constructor(CONFIG_CONSTRUCTOR_PRIORITY)))
static void init_config(void)
{
	int i;
	struct config *config;

	ULOGD("%s", __func__);

	for (i = CONFIG_FIRST; i < CONFIG_NB; i++) {
		config = configs + i;
		if (config->env == NULL)
			continue;
		config->value = getenv(config->env);
		if (config->value == NULL)
			config->value = config->default_value;
	}
}

const char *config_get(enum config_key key)
{
	const char *value;

	value = configs[key].value;

	value = value == NULL ? "" : value;

	return value;
}
