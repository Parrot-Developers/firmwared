/**
 * @file custom_property.c
 * @brief
 *
 * @date 4 déc. 2015
 * @author ncarrier
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ut_string.h>
#include <rs_node.h>
#include <rs_dll.h>

#include "custom_property.h"

#define ULOG_TAG firmwared_custom_property
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_custom_property);

static int custom_property_get(struct folder_property *property,
		struct folder_entity *entity, char **value)
{

	return -ENOSYS;
}

static int custom_property_set(struct folder_property *property,
		struct folder_entity *entity, const char *value)
{

	return -ENOSYS;
}

static int custom_property_geti(struct folder_property *property,
		struct folder_entity *entity, unsigned index,
		char **value)
{

	return -ENOSYS;
}

static int custom_property_seti(struct folder_property *property,
		struct folder_entity *entity, unsigned index,
		const char *value)
{

	return -ENOSYS;
}

bool is_custom_property(const struct folder_property *property)
{
	return property != NULL && (property->get == custom_property_get ||
			property->geti == custom_property_geti);
}

struct folder_property *custom_property_new(const char *name)
{
	int old_errno;
	struct folder_property *property;
	size_t len;
	bool array;

	if (ut_string_is_invalid(name)) {
		errno = -EINVAL;
		return NULL;
	}

	len = strlen(name);
	array = len > 2 && ut_string_match(name + len - 2, "[]");
	property = calloc(1, sizeof(*property));
	if (property == NULL) {
		old_errno = errno;
		ULOGE("calloc: %m");
		errno = old_errno;
		return NULL;
	}
	property->name = strdup(name);
	if (property->name == NULL) {
		old_errno = errno;
		ULOGE("calloc: %m");
		errno = old_errno;
		goto err;
	}

	if (array) {
		property->geti = custom_property_geti;
		property->seti = custom_property_seti;
		/* truncate the [] */
		property->name[len - 2] = '\0';
	} else {
		property->get = custom_property_get;
		property->set = custom_property_set;
	}

	return property;
err:
	custom_property_delete(&property);

	return NULL;
}

int custom_property_cleanup_values(const struct folder_entity *entity)
{

	return -ENOSYS;
}

void custom_property_delete(struct folder_property **property)
{
	struct folder_property *p;

	if (property == NULL || *property == NULL ||
			!is_custom_property(*property))
		return;
	p = *property;

	ut_string_free(&p->name);
	free(*property);
	*property = NULL;
}

static __attribute__((constructor)) void custom_property_init(void)
{
	ULOGD("%s", __func__);

}

static __attribute__((destructor)) void custom_property_cleanup(void)
{
	ULOGD("%s", __func__);

}

