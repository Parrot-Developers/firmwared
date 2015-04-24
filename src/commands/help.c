/**
 * @file help.c
 * @brief
 *
 * @date Apr 24, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <libpomp.h>

#include <ut_string.h>

#define ULOG_TAG firmwared_command_help
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_help);

#include "commands.h"
#include "folders.h"

#define COMMAND_NAME "HELP"

static int help_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *cmd = NULL;
	char __attribute__((cleanup(ut_string_free))) *command_name = NULL;

	ret = pomp_msg_read(msg, "%ms%ms", &cmd, &command_name);
	if (ret < 0) {
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	return firmwared_answer(conn, msg, "%s%s%s", "HELP", command_name,
			command_get_help(command_name));
}

static const struct command help_command = {
		.name = COMMAND_NAME,
		.help = "Shows a little help about a given command.",
		.synopsis = "COMMAND",
		.handler = help_command_handler,
};

static __attribute__((constructor)) void help_init(void)
{
	int ret;

	ret = command_register(&help_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void help_cleanup(void)
{
	int ret;

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
