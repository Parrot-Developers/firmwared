/**
 * @file commands.c
 * @brief
 *
 * @date Apr 27, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_commands
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_commands);

#include "commands.h"

#define COMMAND_NAME "COMMANDS"

static int commands_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	const char *list;

	list = command_list();
	if (list == NULL)
		return -errno;

	return firmwared_answer(conn, msg, "%s%s", "COMMANDS", list);
}

static const struct command commands_command = {
		.name = COMMAND_NAME,
		.help = "List the different commands registered so far.",
		.synopsis = "",
		.handler = commands_command_handler,
};

static __attribute__((constructor)) void commands_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&commands_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void commands_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
