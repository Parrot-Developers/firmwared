/**
 * @file config.c
 * @brief
 *
 * @date Apr 22, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <net/if.h>

#include <unistd.h>
#include <libgen.h>

#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <regex.h>

#include <lua.h>
#include <lauxlib.h>

#define ULOG_TAG firmwared_config
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_config);

#include <ut_string.h>
#include <ut_utils.h>

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
#define INSTANCES_MOUNT_PATH_DEFAULT "/var/cache/firmwared/mount_points/"
#endif /* INSTANCES_MOUNT_PATH_DEFAULT */

#ifndef NET_HOOK_DEFAULT
#define NET_HOOK_DEFAULT "/usr/libexec/firmwared/net.hook"
#endif /* NET_HOOK_DEFAULT */

#ifndef PREVENT_REMOVAL
#define PREVENT_REMOVAL "n"
#endif /* PREVENT_REMOVAL */

#ifndef APPARMOR_PROFILE_DEFAULT
#define APPARMOR_PROFILE_DEFAULT FOLDERS_RESOURCES_DIR_DEFAULT \
	"firmwared.apparmor.profile"
#endif /* APPARMOR_PROFILE_DEFAULT */

#ifndef CONTAINER_INTERFACE
#define CONTAINER_INTERFACE "eth0"
#endif /* CONTAINER_INTERFACE */

#ifndef CURL_HOOK
#define CURL_HOOK "/usr/libexec/firmwared/curl.hook"
#endif /* CURL_HOOK */

#ifndef HOST_INTERFACE_PREFIX
#define HOST_INTERFACE_PREFIX "fd_veth"
#endif /* HOST_INTERFACE_PREFIX */

#ifndef NET_FIRST_TWO_BYTES
#define NET_FIRST_TWO_BYTES "10.202."
#endif /* NET_FIRST_TWO_BYTES */

#ifndef DUMP_PROFILE
#define DUMP_PROFILE "n"
#endif /* DUMP_PROFILE */

#ifndef DISABLE_APPARMOR
#define DISABLE_APPARMOR "n"
#endif /* DISABLE_APPARMOR */

#ifndef VERBOSE_HOOK_SCRIPTS
#define VERBOSE_HOOK_SCRIPTS "n"
#endif /* VERBOSE_HOOK_SCRIPTS */

typedef bool (*validate_cb_t)(const char *value);

struct config {
	char *env;
	char *value;
	char *default_value;
	validate_cb_t valid;
};

static bool valid_path(const char *value)
{
	bool valid;

	if (value == NULL)
		return false;

	valid = access(value, F_OK) == 0;
	if (!valid)
		ULOGE("%s is not a valid path", value);

	return valid;
}


static bool valid_executable(const char *value)
{
	bool valid;

	if (value == NULL)
		return false;

	valid = access(value, X_OK) == 0;
	if (!valid)
		ULOGE("%s is not a valid executable", value);

	return valid;
}


static bool valid_accessible(const char *value)
{
	bool valid;
	char __attribute__((cleanup(ut_string_free))) *tmp = NULL;
	char *bn;

	if (value == NULL)
		return false;

	tmp = strdup(value);
	if (tmp == NULL)
		return false;
	bn = dirname(tmp);

	valid = valid_path(bn);
	if (!valid)
		ULOGE("%s is not accessible", value);

	return valid;
}

static bool is_yes(const char *value)
{
	return ut_string_match("y", value);
}

static bool valid_yes_no(const char *value)
{
	bool valid;

	if (value == NULL)
		return false;

	valid = is_yes(value) || ut_string_match("n", value);
	if (!valid)
		ULOGE("%s is neither \"y\" nor \"n\"", value);

	return valid;
}

static bool valid_interface(const char *value)
{
	bool valid;

	if (value == NULL)
		return false;

	valid = strlen(value) < IFNAMSIZ;
	if (!valid)
		ULOGE("%s is longer than the max interface name length (%d)",
				value, IFNAMSIZ - 1);

	return valid;
}

static bool valid_interface_prefix(const char *value)
{
	bool valid;
	/* -4 gives sufficient room for an id on 1 byte */
	const size_t max = IFNAMSIZ - 4;

	if (value == NULL)
		return false;

	valid = strlen(value) <= max;
	if (!valid)
		ULOGE("%s is longer than the max interface prefix length (%ju)",
				value, (uintmax_t)max);

	return valid;
}

static bool valid_net_first_two_bytes(const char *value)
{
	int ret;
	const char *str_regex = "^([0-9]+)\\.([0-9]+)\\.$";
	regex_t preg;
	char errbuf[0x80];
	bool valid;
	/*
	 * 3 is because the regex has 2 sub expressions, and the entire
	 * expression is put in matches[0]
	 */
	regmatch_t matches[3];
	int len1;
	int len2;
	char byte1[4];
	char byte2[4];

	ret = regcomp(&preg, str_regex, REG_EXTENDED);
	if (ret != 0) {
		regerror(ret, &preg, errbuf, 0x80);
		ULOGE("regcomp: %s", errbuf);
		return false;
	}
	valid = regexec(&preg, value, UT_ARRAY_SIZE(matches), matches, 0) == 0;
	regfree(&preg);
	if (valid) {
		len1 = matches[1].rm_eo - matches[1].rm_so;
		len2 = matches[2].rm_eo - matches[2].rm_so;
		if (len1 > 3 || len2 > 3) {
			valid = false;
		} else {
			snprintf(byte1, 4, "%.*s", len1,
					matches[1].rm_so + value);
			snprintf(byte2, 4, "%.*s", len2,
					matches[2].rm_so + value);

			valid = atoi(byte1) < 256 && atoi(byte2) < 256;
		}
	}
	if (!valid)
		ULOGE("%s should follow the pattern \"X1.X2.\" with X1 and X2 "
				"being two integers in [0, 255] inclusive",
				value);

	return valid;
}

static struct config configs[CONFIG_NB] = {
		[CONFIG_APPARMOR_PROFILE] = {
				.env = CONFIG_KEYS_PREFIX"APPARMOR_PROFILE",
				.default_value = APPARMOR_PROFILE_DEFAULT,
				.valid = valid_accessible,
		},
		[CONFIG_CONTAINER_INTERFACE] = {
				.env = CONFIG_KEYS_PREFIX"CONTAINER_INTERFACE",
				.default_value = CONTAINER_INTERFACE,
				.valid = valid_interface,
		},
		[CONFIG_CURL_HOOK] = {
				.env = CONFIG_KEYS_PREFIX"CURL_HOOK",
				.default_value = CURL_HOOK,
				.valid = valid_executable,
		},
		[CONFIG_DISABLE_APPARMOR] = {
				.env = CONFIG_KEYS_PREFIX"DISABLE_APPARMOR",
				.default_value = DISABLE_APPARMOR,
				.valid = valid_yes_no,
		},
		[CONFIG_DUMP_PROFILE] = {
				.env = CONFIG_KEYS_PREFIX"DUMP_PROFILE",
				.default_value = DUMP_PROFILE,
				.valid = valid_yes_no,
		},
		[CONFIG_HOST_INTERFACE_PREFIX] = {
				.env = CONFIG_KEYS_PREFIX
					"HOST_INTERFACE_PREFIX",
				.default_value = HOST_INTERFACE_PREFIX,
				.valid = valid_interface_prefix,
		},
		[CONFIG_MOUNT_HOOK] = {
				.env = CONFIG_KEYS_PREFIX"MOUNT_HOOK",
				.default_value = MOUNT_HOOK_DEFAULT,
				.valid = valid_executable,
		},
		[CONFIG_MOUNT_PATH] = {
				.env = CONFIG_KEYS_PREFIX"MOUNT_PATH",
				.default_value = INSTANCES_MOUNT_PATH_DEFAULT,
				.valid = valid_path,
		},
		[CONFIG_NET_FIRST_TWO_BYTES] = {
				.env = CONFIG_KEYS_PREFIX"NET_FIRST_TWO_BYTES",
				.default_value = NET_FIRST_TWO_BYTES,
				.valid = valid_net_first_two_bytes,
		},
		[CONFIG_NET_HOOK] = {
				.env = CONFIG_KEYS_PREFIX"NET_HOOK",
				.default_value = NET_HOOK_DEFAULT,
				.valid = valid_executable,
		},
		[CONFIG_PREVENT_REMOVAL] = {
				.env = CONFIG_KEYS_PREFIX"PREVENT_REMOVAL",
				.default_value = PREVENT_REMOVAL,
				.valid = valid_yes_no,
		},
		[CONFIG_RESOURCES_DIR] = {
				.env = CONFIG_KEYS_PREFIX"RESOURCES_DIR",
				.default_value = FOLDERS_RESOURCES_DIR_DEFAULT,
				.valid = valid_path,
		},
		[CONFIG_REPOSITORY_PATH] = {
				.env = CONFIG_KEYS_PREFIX"REPOSITORY_PATH",
				.default_value = FIRMWARE_REPOSITORY_DEFAULT,
				.valid = valid_path,
		},
		[CONFIG_SOCKET_PATH] = {
				.env = CONFIG_KEYS_PREFIX"SOCKET_PATH",
				.default_value = SOCKET_PATH_DEFAULT,
				.valid = valid_accessible,
		},
		[CONFIG_VERBOSE_HOOK_SCRIPTS] = {
				.env = CONFIG_KEYS_PREFIX"VERBOSE_HOOK_SCRIPTS",
				.default_value = VERBOSE_HOOK_SCRIPTS,
				.valid = valid_yes_no,
		},
};

static int lua_error_to_errno(int error)
{
	switch (error) {
	case LUA_OK:
		return 0;

	case LUA_ERRRUN:
		return -ENOEXEC;

	case LUA_ERRMEM:
		return -ENOMEM;

	case LUA_ERRFILE:
		return -EIO;

	case LUA_ERRGCMM:
	case LUA_ERRERR:
	case LUA_YIELD:
	case LUA_ERRSYNTAX:
	default:
		return -EINVAL;
	}
}

static int validate(lua_State *l, const struct config *config)
{
	int ret;

	if (config->value == NULL) {
		ret = errno;
		ULOGE("strdup: %m");
		lua_pushnumber(l, ret);
		return lua_error(l);
	}

	if (config->valid != NULL && !config->valid(config->value)) {
		ULOGE("invalid %s value \"%s\"", config->env, config->value);
		lua_pushnumber(l, EINVAL);
		return lua_error(l);
	}

	return 0;
}

static int read_config(lua_State *l)
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
			validate(l, config);
			continue;
		}
		lua_getglobal(l,  config->env);
		value = lua_tostring(l, -1);
		if (value != NULL) {
			config->value = strdup(value);
			validate(l, config);
			continue;
		}
		config->value = strdup(config->default_value);
		validate(l, config);
	}

	/* dump the config for debug */
	for (i = CONFIG_FIRST; i < CONFIG_NB; i++)
		ULOGD("%s = %s", configs[i].env, configs[i].value);

	return 0;
}

static int check_config(void)
{
	if (config_get_bool(CONFIG_DISABLE_APPARMOR))
		ULOGW("AppArmor disabled, this is dangerous");

	return 0;
}

int config_init(const char *path)
{
	int ret;
	lua_State *l;
	bool is_number;

	l = luaL_newstate();
	if (l == NULL) {
		ULOGE("luaL_newstate() failed");
		return -ENOMEM;
	}

	if (path != NULL) {
		ULOGI("loading configuration from \"%s\"", path);
		ret = luaL_dofile(l, path);
		if (ret != LUA_OK) {
			ret = lua_error_to_errno(ret);
			ULOGE("reading config file: %s", lua_tostring(l, -1));
			goto out;
		}
	}

	lua_pushcfunction(l, read_config);
	ret = lua_pcall(l, 0, 0, 0);
	if (ret != LUA_OK) {
		is_number = lua_isnumber(l, -1);
		ret = -(is_number ? lua_tonumber(l, -1) : EINVAL);
		ULOGE("read_config_file: %s", is_number ? strerror(-ret) :
						lua_tostring(l, -1));
		goto out;
	}

	ret = check_config();

out:
	lua_close(l);

	return ret;
}

enum config_key config_key_from_string(const char *key)
{
	enum config_key i;

	for (i = CONFIG_FIRST; i < CONFIG_NB; i++)
		if (strcasecmp(configs[i].env, key) == 0)
			return i;

	return (enum config_key)-1;
}

const char *config_get(enum config_key key)
{
	const char *value;

	value = configs[key].value;

	value = value == NULL ? "" : value;

	return value;
}

bool config_get_bool(enum config_key key)
{
	const char *value;

	value = config_get(key);

	return value == NULL ? false : is_yes(value);
}

char *config_list_keys(void)
{
	int ret;
	char *keys_list = NULL;
	enum config_key k;
	size_t offset = UT_ARRAY_SIZE(CONFIG_KEYS_PREFIX) - 1;
	char *p;

	for (k = CONFIG_FIRST; k < CONFIG_NB; k++) {
		ret = ut_string_append(&keys_list, "%s ", configs[k].env +
				offset);
		if (ret < 0) {
			ULOGC("ut_string_append");
			errno = -ret;
			return NULL;
		}
	}
	if (keys_list[0] != '\0')
		keys_list[strlen(keys_list) - 1] = '\0';
	for (p = keys_list; *p != '\0'; p++)
		*p = tolower(*p);

	return keys_list;
}

void config_cleanup(void)
{
	int i;

	for (i = CONFIG_FIRST; i < CONFIG_NB; i++)
		ut_string_free(&configs[i].value);
}
