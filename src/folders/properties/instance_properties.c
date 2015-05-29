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

struct folder_property id_property = {
		.name = "id",
		.get = get_id,
};

struct folder_property pid_property = {
		.name = "pid",
		.get = get_pid,
};

struct folder_property state_property = {
		.name = "state",
		.get = get_state,
};

