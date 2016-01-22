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
#define _DEFAULT_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include <sys/apparmor.h>

#define ULOG_TAG apparmor_config
#include <ulog.h>
ULOG_DECLARE_TAG(apparmor_config);

#include <io_process.h>

#include <ut_string.h>
#include <ut_file.h>

#include "apparmor.h"
#include "config.h"
#include "process.h"

#ifndef APPARMOR_PARSER_COMMAND
#define APPARMOR_PARSER_COMMAND "/sbin/apparmor_parser"
#endif /* APPARMOR_PARSER_COMMAND */
#define PROFILE_NAME_PREFIX "firmwared_"
#define PROFILE_NAME_PREFIX_LEN (sizeof(PROFILE_NAME_PREFIX) - 1)
#define PROFILE_NAME_PATTERN PROFILE_NAME_PREFIX "%s"
#define STATIC_PROFILE_PATTERN "@{root}=%s\nprofile "PROFILE_NAME_PATTERN" %s "
#define REMOVE_PROFILE_PATTERN "profile "PROFILE_NAME_PATTERN" {\n}\n"

#define APPARMOR_PROFILES_FILE "/sys/kernel/security/apparmor/profiles"
/* e.g. firmwared_010f0520e5abb2a5a2f0813ebfe2a87a979a1c5c */
#define APPARMOR_PROFILE_NAME_LENGTH (PROFILE_NAME_PREFIX_LEN + 40)

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
		fputs(static_apparmor_profile, stderr);

	return 0;
}

__attribute__ ((format (printf, 2, 3)))
static int vload_profile(const char *action, const char *fmt, ...)
{
	int ret;
	va_list args;
	char __attribute__((cleanup(ut_string_free)))*buf = NULL;
	struct io_process process;
	struct io_process_parameters prms = process_default_parameters;

	va_start(args, fmt);
	ret = vasprintf(&buf, fmt, args);
	va_end(args);
	if (ret < 0) {
		ULOGE("asprintf failure");
		buf = NULL;
		ret = -ENOMEM;
		goto out;
	}

	prms.buffer = buf;
	prms.len = ret;
	prms.copy = false;
	ret = io_process_init_prepare_launch_and_wait(&process, &prms, NULL,
			APPARMOR_PARSER_COMMAND, action, "--quiet", NULL);
	if (config_get_bool(CONFIG_DUMP_PROFILE))
		fputs(buf, stderr);
	ret = 0;
out:

	return ret;
}

int apparmor_load_profile(const char *root, const char *name)
{
	ULOGI("%s(%s, %s)", __func__, root, name);

	return vload_profile("--replace", STATIC_PROFILE_PATTERN, root, name,
			static_apparmor_profile);
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

	return vload_profile("--remove", REMOVE_PROFILE_PATTERN, name);
}

void apparmor_remove_all_firmwared_profiles(void)
{
	FILE __attribute__((cleanup(ut_file_close)))*f = NULL;
	char __attribute__((cleanup(ut_string_free)))*line = NULL;
	ssize_t sret;
	size_t len = 0;
	char *needle;

	f = fopen(APPARMOR_PROFILES_FILE, "r");
	if (f == NULL) {
		ULOGE("opening "APPARMOR_PROFILES_FILE" failed: %m");
		return;
	}
	while ((sret = getline(&line, &len, f)) != -1) {
		if (len < APPARMOR_PROFILE_NAME_LENGTH)
			continue;
		/* strip the end of the line, keep the profile name */
		if ((needle = strchr(line, ' ')) != NULL)
			*needle = '\0';
		apparmor_remove_profile(line + PROFILE_NAME_PREFIX_LEN);
	}
}

void apparmor_cleanup(void)
{
	ut_string_free(&static_apparmor_profile);
}
