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

static int properties_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *list = NULL;
	char __attribute__((cleanup(ut_string_free))) *folder = NULL;

	/* coverity[bad_printf_format_string] */
	ret = pomp_msg_read(msg, FWD_FORMAT_COMMAND_PROPERTIES_READ, &seqnum,
			&folder);
	if (ret < 0) {
		folder = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	list = folder_list_properties(folder);
	if (list == NULL) {
		list = NULL;
		return -errno;
	}

	return firmwared_answer(conn, FWD_ANSWER_PROPERTIES,
			FWD_FORMAT_ANSWER_PROPERTIES, seqnum, folder, list);
}

static const struct command properties_command = {
		.msgid = FWD_COMMAND_PROPERTIES,
		.help = "Asks the server to list the currently registered "
				"properties for the folder FOLDER.",
		.synopsis = "FOLDER",
		.handler = properties_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void properties_cmd_init(void)
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

	ret = command_unregister(properties_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
