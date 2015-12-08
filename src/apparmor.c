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
#define _SVID_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include <sys/apparmor.h>

#define ULOG_TAG apparmor_config
#include <ulog.h>
ULOG_DECLARE_TAG(apparmor_config);

#include <ut_string.h>
#include <ut_file.h>
#include <ut_process.h>

#include "./apparmor.h"
#include "config.h"

#define PROFILE_NAME_PREFIX "firmwared_"
#define PROFILE_NAME_PATTERN PROFILE_NAME_PREFIX "%s"
#define APPARMOR_COMMAND "apparmor_parser --replace --quiet"
#define APPARMOR_REMOVE_COMMAND "apparmor_parser --remove --quiet"
#define STATIC_PROFILE_PATTERN "@{root}=%s\nprofile "PROFILE_NAME_PATTERN" %s "
#define REMOVE_PROFILE_PATTERN "profile "PROFILE_NAME_PATTERN" {\n}\n"

#define APPARMOR_PROFILES_DIR "/sys/kernel/security/apparmor/policy/profiles/"

static char *static_apparmor_profile;

static int filter_dirent_without_firmwared_prefix(const struct dirent *d)
{
	return ut_string_match_prefix(d->d_name, PROFILE_NAME_PREFIX);
}

int apparmor_init(void)
{
	int ret;
	const char *path = NULL;

	ULOGD("%s", __func__);

	if (!aa_is_enabled()) {
		ULOGE("AppArmor is not enabled or installed, please see the "
				"instructions for your distribution to enable "
				"it");
		return -ENOSYS;
	}

	path = config_get(CONFIG_APPARMOR_PROFILE);
	ret = ut_file_to_string("%s", &static_apparmor_profile, path);
	if (ret < 0) {
		ULOGE("ut_file_to_string: %s", strerror(-ret));
		return ret;
	}
	if (config_get_bool(CONFIG_DUMP_PROFILE))
		fprintf(stderr, "%s\n", static_apparmor_profile);

	return 0;
}

__attribute__ ((format (printf, 2, 3)))
static int vload_profile(const char *command, const char *fmt, ...)
{
	int ret;
	FILE *aa_parser_stdin;
	va_list args;

	// TODO replace with io_process
	aa_parser_stdin = ut_process_vpopen(" %s 2>&1  | ulogger -p E -t fd-aa",
			"we", command);
	if (aa_parser_stdin == NULL) {
		ret = -errno;
		ULOGE("popen(%s, \"we\"): %s", command, strerror(-ret));
		return ret;
	}

	va_start(args, fmt);
	ret = vfprintf(aa_parser_stdin, fmt, args);
	va_end(args);
	if (ret < 0) {
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
	char __attribute__((cleanup(ut_string_free)))*profile_name = NULL;

	ret = asprintf(&profile_name, PROFILE_NAME_PATTERN, name);
	if (ret < 0) {
		profile_name = NULL;
		return -ENOMEM;
	}
	ret = aa_change_profile(profile_name);
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

void apparmor_remove_all_firmwared_profiles(void)
{
	int count;
	struct dirent **namelist;
	char *dot;

	count = scandir(APPARMOR_PROFILES_DIR, &namelist,
			filter_dirent_without_firmwared_prefix, NULL);
	if (count == -1) {
		ULOGE("scandir(%s,...): %m", APPARMOR_PROFILES_DIR);
		return;
	}
	while (count--) {
		dot = strchr(namelist[count]->d_name, '.');
		if (dot != NULL)
			*dot = '\0';
		apparmor_remove_profile(namelist[count]->d_name +
				strlen(PROFILE_NAME_PREFIX));
		free(namelist[count]);
	}
	free(namelist);
}

void apparmor_cleanup(void)
{
	ut_string_free(&static_apparmor_profile);
}
