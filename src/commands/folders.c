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

static int folders_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	const char *list;

	list = folders_list();
	if (list == NULL)
		return -errno;

	return pomp_conn_send(conn, pomp_msg_get_id(msg), "%s"
			"%s",
			"FOLDERS",
			list);
}

static const struct command folders_command = {
		.name = COMMAND_NAME,
		.help = "List the different registered folders so far.",
		.handler = folders_command_handler,
};

static __attribute__((constructor)) void folders_init(void)
{
	int ret;

	ret = command_register(&folders_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

__attribute__((destructor)) static void folders_cleanup(void)
{
	int ret;

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
