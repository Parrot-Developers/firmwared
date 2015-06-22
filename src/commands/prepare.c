/**
 * @file prepare.c
 * @brief
 *
 * @date Apr 22, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <openssl/sha.h>

#include <ut_string.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_prepare
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_prepare);

#include "commands.h"
#include "utils.h"
#include "firmwares.h"
#include "instances.h"
#include "folders.h"
#include "utils.h"

#define COMMAND_NAME "PREPARE"

static int prepare_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free)))*cmd = NULL;
	char __attribute__((cleanup(ut_string_free))) *identifier = NULL;
	struct instance *instance;

	ret = pomp_msg_read(msg, "%ms%ms", &cmd, &identifier);
	if (ret < 0) {
		cmd = identifier = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	instance = instance_new(identifier);
	if (instance == NULL) {
		ret = -errno;
		ULOGE("instance_new: %m");
		return ret;
	}
	/* folder_store transfers the ownership of the entity to the folder */
	ret = folder_store(INSTANCES_FOLDER_NAME, instance_to_entity(instance));
	if (ret < 0) {
		instance_delete(&instance, false);
		ULOGE("folder_store: %s", strerror(-ret));
		return ret;
	}

	return firmwared_notify(pomp_msg_get_id(msg), "%s%s%s%s%s",
			"PREPARED", "placeholder",
			"placeholder",
			instance_get_sha1(instance),
			instance_get_name(instance));
}

static const struct command prepare_command = {
		.name = COMMAND_NAME,
		.help = "Creates an instance of the given firmware, in the "
				"READY state.",
		.long_help = "If FIRMWARE_IDENTIFIER doesn't correspond to a "
				"registered firmware, then it is supposed to "
				"be a path to a directory which will be "
				"mounted as the read-only layer for the "
				"prepared instance, this allows to create "
				"instances from the final directory of a "
				"firmware's workspace and is intended for "
				"development.",
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
