/**
 * @file main.c
 * @brief
 *
 * @date Apr 16, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <sys/mount.h>

#include <signal.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>

#include <unistd.h>

#include <regex.h>

#include <ut_process.h>
#include <ut_file.h>
#include <ut_string.h>

#include "apparmor.h"
#include "folders.h"
#include "firmwares.h"
#include "instances.h"
#include "firmwared.h"
#include "commands.h"
#include "config.h"

#define ULOG_TAG firmwared_main
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_main);

#ifndef DEFAULT_CONFIG_FILE
#define DEFAULT_CONFIG_FILE "/etc/firmwared.conf"
#endif /* DEFAULT_CONFIG_FILE */

static void usage(int status)
{
	char name[17];

	fprintf(stderr, "usage: %s [configuration_file]\n",
			ut_process_get_name(name));
	ULOGE("usage: %s [configuration_file]\n", ut_process_get_name(name));

	exit(status);
}

static void clean_subsystems(void)
{
	if (!config_get_bool(CONFIG_DISABLE_APPARMOR))
		apparmor_cleanup();
	instances_cleanup();
	firmwares_cleanup();
	folders_cleanup();
}

static void initial_cleanup_mount_points(void)
{
	int ret;
	char __attribute__((cleanup(ut_string_free)))*line = NULL;
	size_t len = 0;
	ssize_t count;
	const char *mount_path = config_get(CONFIG_MOUNT_PATH);
	FILE __attribute__((cleanup(ut_file_close)))*mounts = NULL;
	char __attribute__((cleanup(ut_string_free)))*str_regex = NULL;
	regex_t preg;
	char errbuf[0x80];
	bool match;
	/*
	 * 2 is because the regex has 1 sub expressions, and the entire
	 * expression is put in matches[0]
	 */
	regmatch_t matches[3];
	const char *path;

	ret = asprintf(&str_regex, "^[^ ]+ (%s/?[a-f0-9]+/[a-z]+) .*$",
			mount_path);
	if (ret == -1) {
		str_regex = NULL;
		ULOGE("asprintf error\n");
		return;
	}
	mounts = fopen("/proc/mounts", "r");
	if (mounts == NULL) {
		ULOGW("failed to open /proc/mounts: %m");
		return;
	}

	ret = regcomp(&preg, str_regex, REG_EXTENDED);
	if (ret != 0) {
		regerror(ret, &preg, errbuf, 0x80);
		ULOGE("regcomp: %s", errbuf);
		return;
	}
	/* unmount all mounted fs' mounted under mount_path */
	while ((count = getline(&line, &len, mounts)) != -1) {
		match = regexec(&preg, line, UT_ARRAY_SIZE(matches), matches, 0)
				== 0;
		if (match) {
			line[matches[1].rm_eo] = '\0';
			path = line + matches[1].rm_so;
			ULOGI("%s: unmount(%s)", __func__, path);
			ret = umount(path);
			if (ret == -1)
				ULOGW("umount(%s): %m", path);
		}
	}

	regfree(&preg);
}

static void initial_cleanup(void)
{
	initial_cleanup_mount_points();
	apparmor_remove_all_firmwared_profiles();
}

static int init_subsystems(void)
{
	int ret;

	ret = folders_init();
	if (ret < 0) {
		ULOGE("folders_init: %s", strerror(-ret));
		return ret;
	}
	ret = firmwares_init();
	if (ret < 0) {
		ULOGE("firmwares_init: %s", strerror(-ret));
		goto err;
	}
	ret = instances_init();
	if (ret < 0) {
		ULOGE("instances_init: %s", strerror(-ret));
		goto err;
	}
	if (!config_get_bool(CONFIG_DISABLE_APPARMOR)) {
		ret = apparmor_init();
		if (ret < 0) {
			ULOGE("apparmor_init: %s", strerror(-ret));
			goto err;
		}
	}

	return 0;
err:
	clean_subsystems();

	return ret;
}

int main(int argc, char *argv[])
{
	int status = EXIT_FAILURE;
	int ret;
	sighandler_t sret;
	const char *commands_list;
	const char *config_file;

	ULOGI("%s[%jd] starting", basename(argv[0]), (intmax_t)getpid());

	if (argc > 2)
		usage(EXIT_FAILURE);

	if (argc == 2)
		config_file = argv[1];
	else if (ut_file_exists(DEFAULT_CONFIG_FILE))
		config_file = DEFAULT_CONFIG_FILE;
	else
		config_file = NULL;

	ret = config_init(config_file);
	if (ret < 0) {
		ULOGE("loading config file %s failed", config_file);
		exit(EXIT_FAILURE);
	}
	initial_cleanup();

	ret = firmwared_init();
	if (ret < 0) {
		ULOGE("firmwared_init: err=%d(%s)", ret, strerror(-ret));
		goto out;
	}
	ret = init_subsystems();
	if (ret < 0) {
		ULOGE("init_subsystems: %s", strerror(-ret));
		goto out;
	}

	commands_list = command_list();
	ULOGD("Commands registered are: %s", commands_list);

	sret = signal(SIGPIPE, SIG_IGN);
	if (sret == SIG_ERR)
		ULOGW("signal: %m");

	firmwared_run();

	status = EXIT_SUCCESS;
out:
	clean_subsystems();
	firmwared_clean();
	config_cleanup();

	ULOGI("%s[%jd] exiting", basename(argv[0]), (intmax_t)getpid());

	return status;
}
