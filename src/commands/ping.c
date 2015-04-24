/**
 * @file ping.c
 * @brief
 *
 * @date Apr 17, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_ping
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_ping);

#include "commands.h"

#define COMMAND_NAME "PING"

static int ping_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	return firmwared_answer(conn, msg, "%s", "PONG");
}

static const struct command ping_command = {
		.name = COMMAND_NAME,
		.help = "Asks the server to give a PONG answer.",
		.synopsis = "",
		.handler = ping_command_handler,
};

static __attribute__((constructor)) void ping_init(void)
{
	int ret;

	ret = command_register(&ping_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void ping_cleanup(void)
{
	int ret;

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
