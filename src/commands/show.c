/**
 * @file show.c
 * @brief
 *
 * @date Apr 21, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <libpomp.h>

#include <ut_string.h>

#define ULOG_TAG firmwared_command_show
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_show);

#include "commands.h"
#include "folders.h"

static int show_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	int ret;
	struct folder_entity *entity;
	char __attribute__((cleanup(ut_string_free))) *folder_name = NULL;
	char __attribute__((cleanup(ut_string_free))) *identifier = NULL;
	char __attribute__((cleanup(ut_string_free))) *info = NULL;

	ret = pomp_msg_read(msg, "%"PRIu32"%ms%ms", &seqnum, &folder_name,
			&identifier);
	if (ret < 0) {
		folder_name = identifier = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}
	entity = folder_find_entity(folder_name, identifier);
	if (entity == NULL) {
		ret = -errno;
		ULOGE("folder_find_entity: %s", strerror(-ret));
		return ret;
	}

	info = folder_get_info(folder_name, identifier);
	if (info == NULL) {
		ret = -errno;
		ULOGE("folder_find_entity: %m");
		return ret;
	}

	return firmwared_answer(conn, FWD_ANSWER_SHOW, "%"PRIu32"%s%s%s%s",
			seqnum, folder_name, folder_entity_get_sha1(entity),
			entity->name, info);
}

static const struct command show_command = {
		.msgid = FWD_COMMAND_SHOW,
		.help = "Asks for all the information on a given entity of a "
				"folder.",
		.synopsis = "FOLDER IDENTIFIER",
		.handler = show_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void show_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&show_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void show_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(show_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
