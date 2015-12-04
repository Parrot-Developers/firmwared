/**
 * @file start.c
 * @brief
 *
 * @date Apr 23, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <ut_string.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_start
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_start);

#include "commands.h"
#include "firmwares.h"
#include "instances.h"
#include "folders.h"

static int start_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *identifier = NULL;
	struct folder_entity *entity;
	struct instance *instance;
	uint32_t msgid = pomp_msg_get_id(msg);
	enum fwd_message ansid = fwd_message_command_answer(msgid);

	/* coverity[bad_printf_format_string] */
	ret = pomp_msg_read(msg, FWD_FORMAT_COMMAND_START_READ, &seqnum,
			&identifier);
	if (ret < 0) {
		identifier = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	entity = folder_find_entity(INSTANCES_FOLDER_NAME, identifier);
	if (entity == NULL)
		return -errno;
	instance = instance_from_entity(entity);
	ret = instance_start(instance);
	if (ret < 0)
		return ret;

	return firmwared_notify(ansid, FWD_FORMAT_ANSWER_STARTED, seqnum,
			instance_get_sha1(instance),
			instance_get_name(instance));
}

static const struct command start_command = {
		.msgid = FWD_COMMAND_START,
		.help = "Starts an previously prepared or stopped instance.",
		.long_help = "Launches an instance, which switches to the "
				"STARTED state and must be in the READY state.",
		.synopsis = "INSTANCE_IDENTIFIER",
		.handler = start_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void start_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&start_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void start_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(start_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
