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

#include "./apparmor.h"
#include "config.h"

#define APPARMOR_COMMAND "apparmor_parser -r -q"
#define APPARMOR_LOG "/tmp/fd.apparmor"

static char *static_apparmor_profile;

int apparmor_init(void)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *path = NULL;

	ULOGD("%s", __func__);

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

int apparmor_enable(const char *root, const char *name)
{
	int ret;
	FILE *aa_parser_stdin;

	ULOGI("%s(%s, %s)", __func__, root, name);

	aa_parser_stdin = popen(APPARMOR_COMMAND" > "APPARMOR_LOG" 2>&1", "we");
	if (aa_parser_stdin == NULL) {
		ret = -errno;
		ULOGE("popen(apparmor_parser, \"we\"): %s", strerror(-ret));
		goto out;
	}

	ret = fprintf(aa_parser_stdin, "@{root}=%s\nprofile %s %s", root, name,
			static_apparmor_profile);
	if (ret < 0) {
		ret = -EIO;
		ULOGE("fprintf to apparmor_parser's stdin failed");
		goto out;
	}
	ret = 0;
out:
	ret = pclose(aa_parser_stdin);
	if (ret == -1) {
		ret = -errno;
		ULOGE("pclose: %s", strerror(-ret));
	} else if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
		ULOGE(APPARMOR_COMMAND" returned %d", WEXITSTATUS(ret));
		ULOGE("one can try to check "APPARMOR_LOG" for errors");
		ret = -EIO;
	}

	if (ret == 0) {
		ret = aa_change_profile(name);
		if (ret < 0) {
			ret = -errno;
			ULOGE("aa_change_profile: %s", strerror(-ret));
			return ret;
		}
	}

	return ret;
}

void apparmor_cleanup(void)
{
	ut_string_free(&static_apparmor_profile);
}
