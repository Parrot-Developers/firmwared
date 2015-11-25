/**
 * @file remount.c
 * @brief
 *
 * @date Aug. 11, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <ut_string.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_remount
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_remount);

#include "commands.h"
#include "instances.h"

static int remount_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *identifier = NULL;
	struct folder_entity *entity;
	struct instance *instance;

	ret = pomp_msg_read(msg, "%"PRIu32"%ms", &seqnum, &identifier);
	if (ret < 0) {
		identifier = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	entity = folder_find_entity(INSTANCES_FOLDER_NAME, identifier);
	if (entity == NULL)
		return -errno;
	instance = instance_from_entity(entity);

	ret = instance_remount(instance);
	if (ret < 0) {
		ULOGE("instance_remount %s", strerror(-ret));
		return ret;
	}

	return firmwared_answer(conn, FWD_ANSWER_REMOUNTED, "%"PRIu32, seqnum);
}

static const struct command remount_command = {
		.msgid = FWD_COMMAND_REMOUNT,
		.help = "Asks to remount the union file system of an instance, "
				"to take into account modifications in the "
				"lower dir (e.g. rebuild of a final dir).",
		.synopsis = "INSTANCE_IDENTIFIER",
		.handler = remount_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void remount_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&remount_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void remount_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(remount_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
