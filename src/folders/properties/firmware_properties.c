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

struct folder_property firmware_properties[] = {
		{
				.name = "path",
				.get = get_path,
		},
		{ /* NULL guard */
				.name = NULL,
				.get = NULL,
				.set = NULL,
				.seti = NULL,
		},
};

