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

#define COMMAND_NAME "START"

static int start_command_handler(struct firmwared *f, struct pomp_conn *conn,
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
	ret = instance_start(f, instance);
	if (ret < 0)
		return ret;

	return pomp_conn_send(conn, pomp_msg_get_id(msg), "%s"
			"%s%s",
			"STARTED",
			instance_get_sha1(instance),
			instance_get_name(instance));
}

static const struct command start_command = {
		.name = COMMAND_NAME,
		.help = "Starts an previously prepared or stopped instance.",
		.handler = start_command_handler,
};

static __attribute__((constructor)) void start_init(void)
{
	int ret;

	ret = command_register(&start_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

__attribute__((destructor)) static void start_cleanup(void)
{
	int ret;

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
