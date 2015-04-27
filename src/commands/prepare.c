/**
 * @file prepare.c
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

#define ULOG_TAG firmwared_command_prepare
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_prepare);

#include "commands.h"
#include "firmwares.h"
#include "instances.h"
#include "folders.h"

#define COMMAND_NAME "PREPARE"

static int prepare_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free)))*cmd = NULL;
	char __attribute__((cleanup(ut_string_free))) *identifier = NULL;
	struct folder_entity *entity;
	struct firmware *firmware;
	struct instance *instance;

	ret = pomp_msg_read(msg, "%ms%ms", &cmd, &identifier);
	if (ret < 0) {
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	entity = folder_find_entity(FIRMWARES_FOLDER_NAME, identifier);
	if (entity == NULL)
		return -errno;
	firmware = firmware_from_entity(entity);
	instance = instance_new(firmware, f);
	if (instance == NULL) {
		ret = -errno;
		ULOGE("instance_new: %m");
		return ret;
	}

	return firmwared_notify(f, pomp_msg_get_id(msg), "%s%s%s%s%s",
			"PREPARED", firmware_get_sha1(firmware),
			firmware_get_name(firmware),
			instance_get_sha1(instance),
			instance_get_name(instance));
}

static const struct command prepare_command = {
		.name = COMMAND_NAME,
		.help = "Create a firmware instance, ready to be launched.",
		.synopsis = "FIRMWARE_IDENTIFIER",
		.handler = prepare_command_handler,
};

static __attribute__((constructor)) void prepare_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&prepare_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void prepare_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
