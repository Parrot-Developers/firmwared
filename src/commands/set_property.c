/**
 * @file set_property.c
 * @brief
 *
 * @date May 29, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <openssl/sha.h>

#include <ut_string.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_set_property
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_set_property);

#include "commands.h"
#include "utils.h"
#include "firmwares.h"
#include "instances.h"
#include "folders.h"

#define COMMAND_NAME "SET_PROPERTY"

static int set_property_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *cmd = NULL;
	char __attribute__((cleanup(ut_string_free))) *folder = NULL;
	char __attribute__((cleanup(ut_string_free))) *identifier = NULL;
	char __attribute__((cleanup(ut_string_free))) *name = NULL;
	char __attribute__((cleanup(ut_string_free))) *value = NULL;
	struct folder_entity *entity;

	ret = pomp_msg_read(msg, "%ms%ms%ms%ms%ms", &cmd, &folder, &identifier,
			&name, &value);
	if (ret < 0) {
		cmd = folder = identifier = name = value = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	entity = folder_find_entity(folder, identifier);
	if (entity == NULL)
		return -errno;
	ret = folder_entity_set_property(entity, name, value);
	if (ret < 0) {
		ULOGE("folder_entity_set_property: %s", strerror(-ret));
		return ret;
	}

	return firmwared_notify(f, pomp_msg_get_id(msg), "%s%s%s%s%s",
			"PROPERTY_SET", folder, identifier, name, value);
}

static const struct command set_property_command = {
		.name = COMMAND_NAME,
		.help = "Sets the value of the property PROPERTY to the value "
				"PROPERTY_VALUE, for the entity whose name or "
				"sha1 is ENTITY_IDENTIFIER from the folder "
				"FOLDER.",
		.long_help = "If the property is an array, append [i] to the "
				"property name to set the i-th item's value. "
				"If i is the index of a non nil element, it "
				"will be replaced, if i is the index after the "
				"last non-nil element, it will be stored in "
				"this position and the array will grow "
				"accordingly. If PROPERTY_VALUE is \"nil\", "
				"then the array will be truncated before the "
				"i-th index.",
		.synopsis = "FOLDER ENTITY_IDENTIFIER PROPERTY_NAME "
				"PROPERTY_VALUE",
		.handler = set_property_command_handler,
};

static __attribute__((constructor)) void set_property_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&set_property_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void set_property_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
