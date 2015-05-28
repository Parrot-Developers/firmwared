/**
 * @file properties.c
 * @brief
 *
 * @date Apr 28, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_properties
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_properties);

#include <ut_string.h>

#include "commands.h"
#include "folders.h"

#define COMMAND_NAME "PROPERTIES"

static int properties_command_handler(struct firmwared *f,
		struct pomp_conn *conn, const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *list = NULL;
	char __attribute__((cleanup(ut_string_free))) *cmd = NULL;
	char __attribute__((cleanup(ut_string_free))) *folder = NULL;

	ret = pomp_msg_read(msg, "%ms%ms", &cmd, &folder);
	if (ret < 0) {
		cmd = folder = NULL; // TODO do that everywhere
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	list = folder_list_properties(folder);
	if (list == NULL) {
		list = NULL;
		return -errno;
	}

	return firmwared_answer(conn, msg, "%s%s%s", folder, COMMAND_NAME,
			list);
}

static const struct command properties_command = {
		.name = COMMAND_NAME,
		.help = "Asks the server to list the currently registered "
				"properties for the folder FOLDER.",
		.synopsis = "FOLDER",
		.handler = properties_command_handler,
};

static __attribute__((constructor)) void properties_cmd_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&properties_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void properties_cmd_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
