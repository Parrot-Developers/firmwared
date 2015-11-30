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

static int ping_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	return firmwared_answer(conn, FWD_ANSWER_PONG, FWD_FORMAT_ANSWER_PONG,
			seqnum);
}

static const struct command ping_command = {
		.msgid = FWD_COMMAND_PING,
		.help = "Asks for the server to answer with a PONG "
				"notification.",
		.synopsis = "PING",
		.handler = ping_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void ping_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&ping_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void ping_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(ping_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
