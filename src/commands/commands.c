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

static int commands_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	const char *list;

	list = command_list();
	if (list == NULL)
		return -errno;

	return firmwared_answer(conn, FWD_ANSWER_COMMANDS, "%"PRIu32"%s",
			seqnum, list);
}

static const struct command commands_command = {
		.msgid = FWD_COMMAND_COMMANDS,
		.help = "List the different commands registered so far.",
		.synopsis = "",
		.handler = commands_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void commands_init(void)
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

	ret = command_unregister(commands_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
