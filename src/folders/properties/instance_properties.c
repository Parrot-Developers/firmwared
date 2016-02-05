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

#include <argz.h>
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
#include "utils.h"
#include "../instances-private.h"

static int get_id(struct folder_property *property,
		struct folder_entity *entity, char **value)
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

static int get_pid(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	int ret;
	struct instance *instance;
	pid_t pid;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	pid = instance->pid;
	ret = asprintf(value, "%jd", (intmax_t)pid);
	if (ret < 0) {
		*value = NULL;
		ULOGE("asprintf error");
		return -ENOMEM;
	}

	return 0;
}

static int get_state(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(instance_state_to_str(instance->state));

	return *value == NULL ? -errno : 0;
}

static int get_firmware_path(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(instance->firmware_path);

	return *value == NULL ? -errno : 0;
}

static int get_root(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(instance->union_mount_point);

	return *value == NULL ? -errno : 0;
}

static int get_pts(struct folder_property *property,
		struct folder_entity *entity, char **value,
		enum pts_index pts_index)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(ptspair_get_path(&instance->ptspair, pts_index));

	return *value == NULL ? -errno : 0;
}

static int get_inner_pts(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	return get_pts(property, entity, value, PTSPAIR_BAR);
}

static int get_outer_pts(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	return get_pts(property, entity, value, PTSPAIR_FOO);
}

static int get_time(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(ctime(&instance->time));
	if (*value == NULL)
		return -errno;
	(*value)[strlen(*value) - 1] = '\0';

	return 0;
}

static int get_interface(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(instance->interface);

	return *value == NULL ? -errno : 0;
}

static int set_interface(struct folder_property *property,
		struct folder_entity *entity, const char *value)
{
	struct instance *instance;

	if (entity == NULL || ut_string_is_invalid(value))
		return -EINVAL;
	instance = to_instance(entity);

	ut_string_free(&instance->interface);

	instance->interface = strdup(value);

	/* TODO validate the input */

	return instance->interface == NULL ? -errno : 0;
}

static int get_stolen_interface(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	if (instance->stolen_interface == NULL)
		*value = strdup("");
	else
		*value = strdup(instance->stolen_interface);

	return *value == NULL ? -errno : 0;
}

static int set_stolen_interface(struct folder_property *property,
		struct folder_entity *entity, const char *value)
{
	struct instance *instance;

	if (entity == NULL || ut_string_is_invalid(value))
		return -EINVAL;
	instance = to_instance(entity);

	ut_string_free(&instance->stolen_interface);

	if (strcmp(value, "nil") == 0)
		return 0;

	instance->stolen_interface = strdup(value);

	/* TODO validate the input */

	return instance->stolen_interface == NULL ? -errno : 0;
}

static int get_stolen_btusb(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	if (instance->stolen_btusb == NULL)
		*value = strdup("");
	else
		*value = strdup(instance->stolen_btusb);

	return *value == NULL ? -errno : 0;
}

static int set_stolen_btusb(struct folder_property *property,
		struct folder_entity *entity, const char *value)
{
	struct instance *instance;

	if (entity == NULL || ut_string_is_invalid(value))
		return -EINVAL;
	instance = to_instance(entity);

	ut_string_free(&instance->stolen_btusb);

	if (strcmp(value, "nil") == 0)
		return 0;

	instance->stolen_btusb = strdup(value);

	/* TODO validate the input */

	return instance->stolen_btusb == NULL ? -errno : 0;
}

static int geti_cmdline(struct folder_property *property,
		struct folder_entity *entity, unsigned index,
		char **value)
{
	struct instance *i;

	if (entity == NULL)
		return -EINVAL;

	i = to_instance(entity);

	return argz_property_geti(i->command_line, i->command_line_len, index,
			value);
}

static int seti_cmdline(struct folder_property *property,
		struct folder_entity *entity, unsigned index,
		const char *value)
{
	struct instance *i;

	if (entity == NULL)
		return -EINVAL;

	i = to_instance(entity);

	return argz_property_seti(&i->command_line, &i->command_line_len, index,
			value);
}

struct folder_property instance_properties[] = {
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
				.name = "root",
				.get = get_root,
		},
		{
				.name = "inner_pts",
				.get = get_inner_pts,
		},
		{
				.name = "outer_pts",
				.get = get_outer_pts,
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
		{
				.name = "stolen_interface",
				.get = get_stolen_interface,
				.set = set_stolen_interface,
		},
		{
				.name = "stolen_btusb",
				.get = get_stolen_btusb,
				.set = set_stolen_btusb,
		},
		{
				.name = "cmdline",
				.geti = geti_cmdline,
				.seti = seti_cmdline,
		},
		{ /* NULL guard */
				.name = NULL,
				.get = NULL,
				.set = NULL,
				.seti = NULL,
		},
};

