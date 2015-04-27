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

#define COMMAND_NAME "QUIT"

static int quit_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	firmwared_stop(f);

	return firmwared_answer(conn, msg, "%s", "BYEBYE");
}

static const struct command quit_command = {
		.name = COMMAND_NAME,
		.help = "Asks the server to terminate.",
		.synopsis = "",
		.handler = quit_command_handler,
};

static __attribute__((constructor)) void quit_init(void)
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

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
