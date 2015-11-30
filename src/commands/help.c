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

static int help_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *command_name = NULL;
	const char *help;

	ret = pomp_msg_read(msg, FWD_FORMAT_COMMAND_HELP_READ, &seqnum,
			&command_name);
	if (ret < 0) {
		command_name = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	help = command_get_help(pomp_msg_get_id(msg));
	if (help == NULL) {
		ret = -errno;
		ULOGE("command_get_help(%s): %m", command_name);
		return -EINVAL;
	}

	return firmwared_answer(conn, FWD_ANSWER_HELP, FWD_FORMAT_ANSWER_HELP,
			seqnum, command_name, help);
}

static const struct command help_command = {
		.msgid = FWD_COMMAND_HELP,
		.help = "Sends back a little help on the command COMMAND.",
		.synopsis = "COMMAND",
		.handler = help_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void help_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&help_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void help_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(help_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
