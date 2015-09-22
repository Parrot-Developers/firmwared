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
#include <signal.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>

#include <unistd.h>

#include <ut_process.h>
#include <ut_file.h>
#include <ut_module.h>

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

static const struct ut_module ulogger_module = {
		.name = "ulogger",
};

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

	if (!ut_module_is_loaded(&ulogger_module)) {
		ret = ut_module_load(&ulogger_module);
		if (ret < 0)
			fprintf(stderr, "ulogger module could'nt be loaded, "
					"continuing, but no log message will be"
					" available");
	}

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
