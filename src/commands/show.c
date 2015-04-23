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

#define COMMAND_NAME "SHOW"

static int show_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	struct folder_entity *entity;
	char __attribute__((cleanup(ut_string_free))) *cmd = NULL;
	char __attribute__((cleanup(ut_string_free))) *folder_name = NULL;
	char __attribute__((cleanup(ut_string_free))) *identifier = NULL;
	char __attribute__((cleanup(ut_string_free))) *info = NULL;

	ret = pomp_msg_read(msg, "%ms%ms%ms", &cmd, &folder_name, &identifier);
	if (ret < 0) {
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

	return pomp_conn_send(conn, firmwared_get_msg_id(f), "%s%"PRIu32
				"%s%s%s%s",
			"SHOW", pomp_msg_get_id(msg),
			folder_name,
			folder_entity_get_sha1(entity),
			entity->name,
			info);
}

static const struct command show_command = {
		.name = COMMAND_NAME,
		.help = "Show detailed information on an entity.",
		.handler = show_command_handler,
};

static __attribute__((constructor)) void show_init(void)
{
	int ret;

	ret = command_register(&show_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

__attribute__((destructor)) static void show_cleanup(void)
{
	int ret;

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
