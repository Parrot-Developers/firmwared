/**
 * @file folders.c
 * @brief
 *
 * @date Apr 21, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_folders
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_folders);

#include "commands.h"
#include "folders.h"

#define COMMAND_NAME "FOLDERS"

static int folders_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	const char *list;

	list = folders_list();
	if (list == NULL)
		return -errno;

	return firmwared_answer(conn, msg, "%s%s", "FOLDERS", list);
}

static const struct command folders_command = {
		.name = COMMAND_NAME,
		.help = "Asks the server to list the currently registered "
				"folders.",
		.synopsis = "",
		.handler = folders_command_handler,
};

static __attribute__((constructor)) void folders_cmd_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&folders_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void folders_cmd_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
