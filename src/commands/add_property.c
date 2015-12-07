/**
 * @file add_property.c
 * @brief
 *
 * @date Dec., 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <ut_string.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_add_property
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_add_property);

#include "commands.h"
#include "folders.h"

static int add_property_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *folder = NULL;
	char __attribute__((cleanup(ut_string_free))) *property_name = NULL;
	uint32_t msgid = pomp_msg_get_id(msg);
	enum fwd_message ansid = fwd_message_command_answer(msgid);

	ret = pomp_msg_read(msg, FWD_FORMAT_COMMAND_ADD_PROPERTY_READ, &seqnum,
			&folder, &property_name);
	if (ret < 0) {
		folder = property_name = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	ret = folder_add_property(folder, property_name);
	if (ret < 0) {
		ULOGE("folder_entity_add_property: %s", strerror(-ret));
		return ret;
	}

	/* coverity[bad_printf_format_string] */
	return firmwared_notify(ansid, FWD_FORMAT_ANSWER_PROPERTY_ADDED, seqnum,
			folder, property_name);
}

static const struct command add_property_command = {
		.msgid = FWD_COMMAND_ADD_PROPERTY,
		.help = "Adds the custom property PROPERTY to the folder "
				"FOLDER.",
		.long_help = "The initial value will be \"\".",
		.synopsis = "FOLDER PROPERTY_NAME",
		.handler = add_property_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void add_property_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&add_property_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void add_property_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(add_property_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
