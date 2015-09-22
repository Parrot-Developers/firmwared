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

static int drop_command_handler(struct pomp_conn *conn,
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
		cmd = folder = identifier = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	entity = folder_find_entity(folder, identifier);
	if (entity == NULL)
		return -errno;
	ret = folder_entity_get_property(entity, "name", &name);
	if (ret < 0)
		return ret;
	sha1 = strdup(folder_entity_get_sha1(entity));
	if (name == NULL || sha1 == NULL)
		return -ENOMEM;

	ret = folder_drop(folder, entity);
	if (ret < 0) {
		ULOGE("folder_drop: %s", strerror(-ret));
		return ret;
	}

	return firmwared_notify(pomp_msg_get_id(msg), "%s%s%s%s", "DROPPED",
			folder, sha1, name);
}

static const struct command drop_command = {
		.name = COMMAND_NAME,
		.help = "Removes an entity from a folder.",
		.long_help = "if the entity is an instance, it must be in the "
				"READY state. It's pid 1 will be killed and "
				"it's run artifacts will be removed if "
				"FIRMWARED_PREVENT_REMOVAL isn't set to \"y\".",
		.synopsis = "FOLDER IDENTIFIER",
		.handler = drop_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void drop_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&drop_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void drop_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
