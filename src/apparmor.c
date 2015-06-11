/**
 * @file apparmor.c
 * @brief
 *
 * @date May 22, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <sys/apparmor.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ULOG_TAG apparmor_config
#include <ulog.h>
ULOG_DECLARE_TAG(apparmor_config);

#include <ut_string.h>
#include <ut_file.h>
#include <ut_process.h>

#include "./apparmor.h"
#include "config.h"

#define APPARMOR_COMMAND "apparmor_parser --replace --quiet"
#define APPARMOR_REMOVE_COMMAND "apparmor_parser --remove --quiet"
#define APPARMOR_LOG "/tmp/fd.apparmor"
#define STATIC_PROFILE_PATTERN "@{root}=%s\nprofile firmwared_%s %s "
#define REMOVE_PROFILE_PATTERN "profile firmwared_%s {\n}\n"

#define APPARMOR_ENABLED_FILE "/sys/module/apparmor/parameters/enabled"


static char *static_apparmor_profile;

static bool apparmor_is_enabled(void)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *status = NULL;

	ret = ut_file_to_string(APPARMOR_ENABLED_FILE, &status);
	if (ret < 0) {
		status = NULL;
		return false;
	}

	return ut_string_match(status, "Y\n");
}

int apparmor_init(void)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *path = NULL;

	ULOGD("%s", __func__);

	if (!apparmor_is_enabled()) {
		ULOGE("AppArmor is not enabled or installed, please see the "
				"instructions for your distribution to enable "
				"it");
		return -ENOSYS;
	}

	ret = asprintf(&path, "%s/firmwared.apparmor.profile",
			config_get(CONFIG_RESOURCES_DIR));
	if (ret < 0) {
		path = NULL;
		ULOGE("asprintf error");
		return -EINVAL;
	}
	ret = ut_file_to_string(path, &static_apparmor_profile);
	if (ret < 0) {
		ULOGE("ut_file_to_string: %s", strerror(-ret));
		return ret;
	}

	return 0;
}

__attribute__ ((format (printf, 2, 3)))
static int vload_profile(const char *command, const char *fmt, ...)
{
	int ret;
	FILE *aa_parser_stdin;
	va_list args;

	aa_parser_stdin = ut_process_vpopen("date >> "APPARMOR_LOG"; %s >> "
			APPARMOR_LOG" 2>&1", "we",
			command);
	if (aa_parser_stdin == NULL) {
		ret = -errno;
		ULOGE("popen(%s, \"we\"): %s", command, strerror(-ret));
		goto out;
	}

	va_start(args, fmt);
	ret = vfprintf(aa_parser_stdin, fmt, args);
	va_end(args);
	if (ret < 0) {
		ret = -EIO;
		ULOGE("fprintf to apparmor_parser's stdin failed");
		goto out;
	}
	if (config_get_bool(CONFIG_DUMP_PROFILE)) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
	}
	ret = 0;
out:
	ret = pclose(aa_parser_stdin);
	if (ret == -1) {
		ret = -errno;
		ULOGE("pclose: %s", strerror(-ret));
	} else if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
		ULOGE("%s returned %d", command, WEXITSTATUS(ret));
		ULOGE("one can try to check "APPARMOR_LOG" for errors");
		ret = -EIO;
	}

	return ret;
}

int apparmor_load_profile(const char *root, const char *name)
{
	ULOGI("%s(%s, %s)", __func__, root, name);

	return vload_profile(APPARMOR_COMMAND, STATIC_PROFILE_PATTERN, root,
			name, static_apparmor_profile);
}

int apparmor_change_profile(const char *name)
{
	int ret;

	ret = aa_change_profile(name);
	if (ret < 0) {
		ret = -errno;
		ULOGE("aa_change_profile: %s", strerror(-ret));
	}

	return ret;
}

int apparmor_remove_profile(const char *name)
{
	ULOGI("%s(%s)", __func__, name);

	return vload_profile(APPARMOR_REMOVE_COMMAND, REMOVE_PROFILE_PATTERN,
			name);
}

void apparmor_cleanup(void)
{
	ut_string_free(&static_apparmor_profile);
}
