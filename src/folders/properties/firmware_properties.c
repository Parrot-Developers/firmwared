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

static int get_product(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct firmware *firmware;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	firmware = to_firmware(entity);

	if (firmware->product == NULL) {
		ULOGW("no product property for firmware %s",
				firmware_get_name(firmware));
		*value = strdup("(none)");
	} else {
		*value = strdup(firmware->product);
	}

	return *value == NULL ? -errno : 0;
}

static int get_hardware(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct firmware *firmware;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	firmware = to_firmware(entity);

	if (firmware->hardware == NULL) {
		ULOGW("no hardware property for firmware %s",
				firmware_get_name(firmware));
		*value = strdup("(none)");
	} else {
		*value = strdup(firmware->hardware);
	}

	return *value == NULL ? -errno : 0;
}

struct folder_property firmware_properties[] = {
		{
				.name = "path",
				.get = get_path,
		},
		{
				.name = "product",
				.get = get_product,
		},
		{
				.name = "hardware",
				.get = get_hardware,
		},
		{ /* NULL guard */
				.name = NULL,
				.get = NULL,
				.set = NULL,
				.seti = NULL,
		},
};

