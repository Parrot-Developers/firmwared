/**
 * @file quit.c
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

#define ULOG_TAG firmwared_command_quit
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_quit);

#include "commands.h"

static int quit_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	firmwared_stop();

	return firmwared_notify(pomp_msg_get_id(msg), "%"PRIu32, seqnum);

}

static const struct command quit_command = {
		.msgid = FWD_COMMAND_QUIT,
		.help = "Asks firmwared to exit.",
		.synopsis = "",
		.handler = quit_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void quit_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&quit_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void quit_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(quit_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
