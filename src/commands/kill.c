/**
 * @file kill.c
 * @brief
 *
 * @date Apr 24, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <ut_string.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_kill
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_kill);

#include "commands.h"
#include "firmwares.h"
#include "instances.h"
#include "folders.h"

#define COMMAND_NAME "KILL"

static int kill_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *cmd = NULL;
	char __attribute__((cleanup(ut_string_free))) *identifier = NULL;
	struct folder_entity *entity;
	struct instance *instance;

	ret = pomp_msg_read(msg, "%ms%ms", &cmd, &identifier);
	if (ret < 0) {
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	entity = folder_find_entity(INSTANCES_FOLDER_NAME, identifier);
	if (entity == NULL)
		return -errno;
	instance = instance_from_entity(entity);

	return instance_kill(instance, pomp_msg_get_id(msg));
}

static const struct command kill_command = {
		.name = COMMAND_NAME,
		.help = "Kills a running instance.",
		.long_help = "Searches for the instance whose sha1 or name is "
				"INSTANCE_IDENTIFIER and kills it. All the "
				"processes are killed, the instance is still "
				"registered and it's rw aufs layer is still "
				"present. The instance must be in the STARTED "
				"state.\nThe instance switches to the STOPPING "
				"state, before switching back to the READY "
				"state.",
		.synopsis = "INSTANCE_IDENTIFIER",
		.handler = kill_command_handler,
};

static __attribute__((constructor)) void kill_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&kill_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void kill_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
