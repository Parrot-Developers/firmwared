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

#include "firmwares.h"
#include "firmwared.h"
#include "commands.h"
#include "config.h"

#define ULOG_TAG firmwared_main
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_main);

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
	firmwares_cleanup();
}

static int init_subsystems(void)
{
	int ret;

	ret = firmwares_init();
	if (ret < 0) {
		ULOGE("firmwares_init: %s", strerror(-ret));
		return ret;
	}

	return 0;
//err:
//	clean_subsystems();
//
//	return ret;
}

int main(int argc, char *argv[])
{
	int status = EXIT_FAILURE;
	int ret;
	struct firmwared ctx;
	sighandler_t sret;
	const char *commands_list;
	const char *config_file;

	ULOGI("%s[%jd] starting", basename(argv[0]), (intmax_t)getpid());

	if (argc > 2)
		usage(EXIT_FAILURE);

	if (argc == 2) {
		config_file = argv[1];
		ret = config_load_file(config_file);
		if (ret < 0) {
			ULOGE("loading config file %s failed", config_file);
			exit(EXIT_FAILURE);
		}
	}

	ret = init_subsystems();
	if (ret < 0) {
		ULOGE("init_subsystems: %s", strerror(-ret));
		goto out;
	}
	ret = firmwared_init(&ctx);
	if (ret < 0) {
		ULOGE("firmwared_init: err=%d(%s)", ret, strerror(-ret));
		goto out;
	}

	commands_list = command_list();
	ULOGD("Commands registered are: %s", commands_list);

	sret = signal(SIGPIPE, SIG_IGN);
	if (sret == SIG_ERR)
		ULOGW("signal: %m");

	firmwared_run(&ctx);

	status = EXIT_SUCCESS;
out:
	clean_subsystems();
	firmwared_clean(&ctx);

	ULOGI("%s[%jd] exiting", basename(argv[0]), (intmax_t)getpid());

	return status;
}
