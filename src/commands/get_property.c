/**
 * @file get_property.c
 * @brief
 *
 * @date May, 2015
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

#define ULOG_TAG firmwared_command_get_property
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_get_property);

#include "commands.h"
#include "utils.h"
#include "firmwares.h"
#include "instances.h"
#include "folders.h"

#define COMMAND_NAME "GET_PROPERTY"

static int get_property_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *cmd = NULL;
	char __attribute__((cleanup(ut_string_free))) *folder = NULL;
	char __attribute__((cleanup(ut_string_free))) *identifier = NULL;
	char __attribute__((cleanup(ut_string_free))) *property_name = NULL;
	char __attribute__((cleanup(ut_string_free))) *value = NULL;
	struct folder_entity *entity;

	ret = pomp_msg_read(msg, "%ms%ms%ms%ms", &cmd, &folder, &identifier,
			&property_name);
	if (ret < 0) {
		cmd = folder = identifier = property_name = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	entity = folder_find_entity(folder, identifier);
	if (entity == NULL)
		return -errno;
	ret = folder_entity_get_property(entity, property_name, &value);
	if (ret < 0) {
		ULOGE("folder_entity_get_property: %s", strerror(-ret));
		return ret;
	}

	return firmwared_notify(pomp_msg_get_id(msg), "%s%s%s%s%s",
			COMMAND_NAME, folder, identifier, property_name, value);
}

static const struct command get_property_command = {
		.name = COMMAND_NAME,
		.help = "Retrieves the value of the property PROPERTY for the "
				"entity whose name or sha1 is ENTITY_IDENTIFIER"
				" from the folder FOLDER.",
		.long_help = "If the property is an array, both indexed and "
				"non-indexed accesses are allowed. "
				"In the non indexed case, all the content of "
				"the array will be retrieved, in the indexed "
				"access case, one must suffix the property "
				"name with [i] to retrieve the i-th value.",
		.synopsis = "FOLDER ENTITY_IDENTIFIER PROPERTY_NAME",
		.handler = get_property_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void get_property_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&get_property_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void get_property_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
