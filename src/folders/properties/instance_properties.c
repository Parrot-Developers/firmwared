/**
 * @file instance_properties.c
 * @brief
 *
 * @date May 28, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <inttypes.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define ULOG_TAG firmwared_instance_properties
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_instance_properties);

#include <ut_string.h>

#include <ptspair.h>

#include "instance_properties.h"
#include "instances.h"
#include "../instances-private.h"

static int get_id(struct folder_entity *entity, char **value)
{
	int ret;
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	ret = asprintf(value, "%"PRIu8, instance->id);
	if (ret < 0) {
		*value = NULL;
		ULOGE("asprintf error");
		return -ENOMEM;
	}

	return 0;
}

static int get_pid(struct folder_entity *entity, char **value)
{
	int ret;
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	ret = asprintf(value, "%jd", (intmax_t)instance->pid);
	if (ret < 0) {
		*value = NULL;
		ULOGE("asprintf error");
		return -ENOMEM;
	}

	return 0;
}

static int get_state(struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(instance_state_to_str(instance->state));

	return *value == NULL ? -errno : 0;
}

static int get_firmware_path(struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(instance->firmware_path);

	return *value == NULL ? -errno : 0;
}

static int get_base_workspace(struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(instance->base_workspace);

	return *value == NULL ? -errno : 0;
}

static int get_pts(struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(ptspair_get_path(&instance->ptspair, PTSPAIR_FOO));

	return *value == NULL ? -errno : 0;
}

static int get_firmware_sha1(struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(instance->firmware_sha1);

	return *value == NULL ? -errno : 0;
}

static int get_time(struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(ctime(&instance->time));
	if (value == NULL)
		return -errno;
	(*value)[strlen(*value) - 1] = '\0';

	return *value == NULL ? -errno : 0;
}

static int get_interface(struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(instance->interface);

	return *value == NULL ? -errno : 0;
}

static int set_interface(struct folder_entity *entity, const char *value)
{
	struct instance *instance;

	if (entity == NULL || ut_string_is_invalid(value))
		return -EINVAL;
	instance = to_instance(entity);

	ut_string_free(&instance->interface);

	instance->interface = strdup(value);

	// TODO validate the input

	return instance->interface == NULL ? -errno : 0;
}

struct folder_property properties[] = {
		{
				.name = "id",
				.get = get_id,
		},
		{
				.name = "pid",
				.get = get_pid,
		},
		{
				.name = "state",
				.get = get_state,
		},
		{
				.name = "firmware_path",
				.get = get_firmware_path,
		},
		{
				.name = "base_workspace",
				.get = get_base_workspace,
		},
		{
				.name = "pts",
				.get = get_pts,
		},
		{
				.name = "firmware_sha1",
				.get = get_firmware_sha1,
		},
		{
				.name = "time",
				.get = get_time,
		},
		{
				.name = "interface",
				.get = get_interface,
				.set = set_interface,
		},
		{ /* NULL guard */
				.name = NULL,
				.get = NULL,
		},
};

int instance_properties_register(void)
{
	int ret;
	struct folder_property *property = properties;

	for (property = properties; property->name != NULL; property++) {
		ret = folder_register_property(INSTANCES_FOLDER_NAME, property);
		if (ret < 0) {
			ULOGE("folder_register_property: %s", strerror(-ret));
			return ret;
		}
	}

	return 0;
}
