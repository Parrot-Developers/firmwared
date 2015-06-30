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
	pid_t pid;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	pid = io_src_pid_get_pid(&instance->pid_src);
	ret = asprintf(value, "%jd", (intmax_t)pid);
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

static int get_pts(struct folder_entity *entity, char **value,
		enum pts_index pts_index)
{
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	*value = strdup(ptspair_get_path(&instance->ptspair, pts_index));

	return *value == NULL ? -errno : 0;
}

static int get_inner_pts(struct folder_entity *entity, char **value)
{
	return get_pts(entity, value, PTSPAIR_BAR);
}

static int get_outer_pts(struct folder_entity *entity, char **value)
{
	return get_pts(entity, value, PTSPAIR_FOO);
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

static char *get_argz_i(char *argz, size_t argz_len, int i)
{
	char *entry = NULL;

	for (entry = argz_next(argz, argz_len, entry); entry != NULL && i != 0;
			entry = argz_next(argz, argz_len, entry), i--);

	return entry;
}

static int geti_cmdline(struct folder_entity *entity, unsigned index,
		char **value)
{
	struct instance *i;
	size_t count;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	i = to_instance(entity);
	count = argz_count(i->command_line, i->command_line_len);
	if (index > count)
			return -ERANGE;

	if (index == count)
		*value = strdup("nil");
	else
		*value = strdup(get_argz_i(i->command_line, i->command_line_len,
				index));

	return *value == NULL ? -errno : 0;
}

static int seti_cmdline(struct folder_entity *entity, unsigned index,
		const char *value)
{
	struct instance *i;
	size_t count;
	char *entry;
	char *before;

	if (entity == NULL || ut_string_is_invalid(value))
		return -EINVAL;
	i = to_instance(entity);
	count = argz_count(i->command_line, i->command_line_len);

	if (index > count) {
		ULOGE("index %u above array size %ju", index, (uintmax_t)count);
		return -ERANGE;
	}

	if (index == count) {
		/* truncation required at the same size the cmdline has */
		if (strcmp(value, "nil") == 0)
			return 0;

		/* append */
		return -argz_add(&i->command_line, &i->command_line_len, value);
	}

	/* truncate */
	entry = get_argz_i(i->command_line, i->command_line_len, index);
	if (strcmp(value, "nil") == 0) {
		/* changing the size suffices */
		i->command_line_len = entry - i->command_line;
		return 0;
	}

	/* delete and insert */
	argz_delete(&i->command_line, &i->command_line_len, entry);
	entry = NULL; /* /!\ entry is now invalid */
	before = get_argz_i(i->command_line, i->command_line_len, index);

	return argz_insert(&i->command_line, &i->command_line_len, before,
			value);
}

static struct folder_property properties[] = {
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
