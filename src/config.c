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
#include <string.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>

#define ULOG_TAG firmwared_config
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_config);

#include <ut_string.h>

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

#ifndef PREVENT_REMOVAL
#define PREVENT_REMOVAL "n"
#endif /* PREVENT_REMOVAL */

struct config {
	char *env;
	char *value;
	char *default_value;
	// TODO add a validation callback
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
		[CONFIG_PREVENT_REMOVAL] = {
				.env = "FIRMWARED_PREVENT_REMOVAL",
				.default_value = PREVENT_REMOVAL,
		},
};

static int lua_error_to_errno(int error)
{
	switch (error) {
	case LUA_OK:
		return 0;

	case LUA_ERRRUN:
		return ENOEXEC;

	case LUA_ERRMEM:
		return ENOMEM;

	case LUA_ERRFILE:
		return EIO;

	case LUA_ERRGCMM:
	case LUA_ERRERR:
	case LUA_YIELD:
	case LUA_ERRSYNTAX:
	default:
		return EINVAL;
	}
}

static int l_read_config_file(lua_State *l)
{
	int i;
	struct config *config;
	const char *value;

	ULOGD("%s", __func__);

	/* precedence is env >> config file >> compilation default value */
	for (i = CONFIG_FIRST; i < CONFIG_NB; i++) {
		config = configs + i;
		value = getenv(config->env);
		if (value != NULL) {
			config->value = strdup(value);
			continue;
		}
		lua_getglobal(l,  config->env);
		value = lua_tostring(l, -1);
		if (value != NULL) {
			config->value = strdup(value);
			continue;
		}
		config->value = strdup(config->default_value);
		// TODO abort on strdup error
	}

	/* dump the config for debug */
	for (i = CONFIG_FIRST; i < CONFIG_NB; i++)
		ULOGD("%s = %s", configs[i].env, configs[i].value);

	return 0;
}

int config_init(const char *path)
{
	int ret;
	lua_State *l;

	l = luaL_newstate();
	if (l == NULL) {
		ULOGE("luaL_newstate() failed");
		return -ENOMEM;
	}

	if (path != NULL) {
		ret = luaL_dofile(l, path);
		if (ret != LUA_OK) {
			ULOGE("reading config file: %s", lua_tostring(l, -1));
			goto out;
		}
	}

	lua_pushcfunction(l, l_read_config_file);
	ret = lua_pcall(l, 0, 0, 0);
	if (ret != LUA_OK) {
		ULOGE("retrieving config options from config file: %s",
				lua_tostring(l, -1));
		goto out;
	}
out:
	lua_close(l);

	return -lua_error_to_errno(ret);
}

const char *config_get(enum config_key key)
{
	const char *value;

	value = configs[key].value;

	value = value == NULL ? "" : value;

	return value;
}

void config_cleanup(void)
{
	int i;

	for (i = CONFIG_FIRST; i < CONFIG_NB; i++)
		ut_string_free(&configs[i].value);
}
