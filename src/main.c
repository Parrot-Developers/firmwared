/**
 * @file main.c
 * @brief
 *
 * @date Apr 16, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <libgen.h>

#include <unistd.h>

#include "firmwared.h"
#include "commands.h"

#define ULOG_TAG firmwared_main
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_main);

int main(int argc, char *argv[])
{
	int status = EXIT_FAILURE;
	int ret;
	struct firmwared ctx;

	ULOGI("%s[%jd] starting", basename(argv[0]), (intmax_t)getpid());

	ret = firmwared_init(&ctx);
	if (ret < 0) {
		ULOGE("init_main: err=%d(%s)", ret, strerror(-ret));
		goto out;
	}

	command_list();

	firmwared_run(&ctx);

	status = EXIT_SUCCESS;
out:
	firmwared_clean(&ctx);

	ULOGI("%s[%jd] exiting", basename(argv[0]), (intmax_t)getpid());

	return status;
}
