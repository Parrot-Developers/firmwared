/**
 * @file version.c
 * @brief
 *
 * @date Apr 27, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <libpomp.h>

#include <ut_string.h>

#include <firmwared-revision.h>

#define ULOG_TAG firmwared_command_version
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_version);

#include "commands.h"

#define COMMAND_NAME "VERSION"

static int version_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *version;

	ret = asprintf(&version, "Compilation time: "__DATE__" - "__TIME__"\n"
			"Version: "ALCHEMY_REVISION_FIRMWARED"\n");
	if (ret < 0) {
		ULOGE("asprintf error");
		return -ENOMEM;
	}

	return firmwared_answer(conn, msg, "%s%s", "VERSION", version);
}

static const struct command version_command = {
		.name = COMMAND_NAME,
		.help = "Asks the server to describe it's current version.",
		.synopsis = "",
		.handler = version_command_handler,
};

static __attribute__((constructor)) void version_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&version_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void version_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
