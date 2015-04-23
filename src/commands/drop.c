/**
 * @file drop.c
 * @brief
 *
 * @date Apr 22, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <ut_string.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_drop
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_drop);

#include "commands.h"
#include "firmwares.h"
#include "instances.h"
#include "folders.h"

#define COMMAND_NAME "DROP"

static int drop_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *cmd = NULL;
	char __attribute__((cleanup(ut_string_free))) *folder = NULL;
	char __attribute__((cleanup(ut_string_free))) *identifier = NULL;
	char __attribute__((cleanup(ut_string_free))) *name = NULL;
	char __attribute__((cleanup(ut_string_free))) *sha1 = NULL;
	struct folder_entity *entity;

	ret = pomp_msg_read(msg, "%ms%ms%ms", &cmd, &folder, &identifier);
	if (ret < 0) {
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	entity = folder_find_entity(folder, identifier);
	if (entity == NULL)
		return -errno;
	name = strdup(folder_entity_get_name(entity));
	sha1 = strdup(folder_entity_get_sha1(entity));
	if (name == NULL || sha1 == NULL)
		return -ENOMEM;

	ret = folder_drop(folder, entity);
	if (ret < 0) {
		ULOGE("folder_drop: %s", strerror(-ret));
		return ret;
	}

	return pomp_conn_send(conn, firmwared_get_msg_id(f), "%s%"PRIu32
			"%s%s%s",
			"DROPPED", pomp_msg_get_id(msg),
			folder,
			sha1,
			name);
}

static const struct command drop_command = {
		.name = COMMAND_NAME,
		.help = "Drop an entity.",
		.handler = drop_command_handler,
};

static __attribute__((constructor)) void drop_init(void)
{
	int ret;

	ret = command_register(&drop_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

__attribute__((destructor)) static void drop_cleanup(void)
{
	int ret;

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
