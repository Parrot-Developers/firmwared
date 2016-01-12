/**
 * @file firmware_properties.c
 * @brief
 *
 * @date June 30, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <ut_string.h>

#define ULOG_TAG firmwared_firmware_properties
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_firmware_properties);

#include "firmware_properties.h"
#include "firmwares.h"
#include "../firmwares-private.h"

static int get_path(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct firmware *firmware;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	firmware = to_firmware(entity);

	*value = strdup(firmware->path);

	return *value == NULL ? -errno : 0;
}

static int get_post_prepare_instance_command(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct firmware *firmware;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	firmware = to_firmware(entity);

	if (firmware->post_prepare_instance_command == NULL)
		*value = strdup("");
	else
		*value = strdup(firmware->post_prepare_instance_command);

	return *value == NULL ? -errno : 0;
}

static int set_post_prepare_instance_command(struct folder_property *property,
		struct folder_entity *entity, const char *value)
{
	struct firmware *firmware;

	if (entity == NULL || ut_string_is_invalid(value))
		return -EINVAL;
	firmware = to_firmware(entity);

	ut_string_free(&firmware->post_prepare_instance_command);

	if (strcmp(value, "nil") == 0)
		return 0;

	firmware->post_prepare_instance_command = strdup(value);

	return firmware->post_prepare_instance_command == NULL ? -errno : 0;
}

static int get_uuid(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct firmware *firmware;
	const char *uuid;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	firmware = to_firmware(entity);

	uuid = firmware_get_uuid(firmware);
	if (uuid == NULL)
		return -errno;
	*value = strdup(uuid);

	return *value == NULL ? -errno : 0;
}

struct folder_property firmware_properties[] = {
		{
				.name = "path",
				.get = get_path,
		},
		{
				.name = "post_prepare_instance_command",
				.get = get_post_prepare_instance_command,
				.set = set_post_prepare_instance_command,
		},
		{
				.name = "uuid",
				.get = get_uuid,
		},
		{ /* NULL guard */
				.name = NULL,
				.get = NULL,
				.set = NULL,
				.seti = NULL,
		},
};

