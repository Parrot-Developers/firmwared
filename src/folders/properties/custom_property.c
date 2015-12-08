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
#include <argz.h>

#include <ut_string.h>
#include <rs_node.h>
#include <rs_dll.h>

#include "utils.h"
#include "custom_property.h"

#define ULOG_TAG firmwared_custom_property
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_custom_property);

struct custom_property_storage {
	struct rs_node node;
	struct folder_property *property;
	struct folder_entity *entity;
	char *argz;
	size_t argz_len;
};

#define to_custom_property_storage(n) ut_container_of(n, \
		struct custom_property_storage, node)

static struct rs_dll custom_property_storages;

static struct custom_property_storage *next_from(struct rs_dll *dll,
		struct custom_property_storage *from)
{
	struct rs_node *node = NULL;

	node = rs_dll_next_from(&custom_property_storages, &from->node);
	if (node == NULL)
		return NULL;

	return to_custom_property_storage(node);
}

static struct custom_property_storage *find_or_create(
		struct folder_property *property,
		struct folder_entity *entity)
{
	struct custom_property_storage *storage = NULL;

	if (property == NULL || entity == NULL) {
		errno = ENOENT;
		return NULL;
	}

	/* return the property storage if found */
	while ((storage = next_from(&custom_property_storages, storage)))
		if (ut_string_match(property->name, storage->property->name)
				&& ut_string_match(entity->name,
						storage->entity->name))
			return storage;

	/* otherwise, allocate it and store it */
	storage = calloc(sizeof(struct custom_property_storage), 1);
	if (storage == NULL)
		return NULL;

	storage->entity = entity;
	storage->property = property;

	rs_dll_enqueue(&custom_property_storages, &storage->node);

	return storage;
}

static int custom_property_geti(struct folder_property *property,
		struct folder_entity *entity, unsigned i, char **value)
{
	int ret;
	struct custom_property_storage *s = find_or_create(property, entity);

	if (s == NULL) {
		ret = -errno;
		ULOGE("find_or_create: %m");
		return ret;
	}

	return argz_property_geti(s->argz, s->argz_len, i, value);
}

static int custom_property_get(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	int ret;

	ret = custom_property_geti(property, entity, 0, value);
	if (ret < 0)
		return ret;

	if (*value == NULL) {
		*value = strdup("");
		if (*value == NULL)
			return -errno;
	}

	return 0;
}

static int custom_property_seti(struct folder_property *property,
		struct folder_entity *entity, unsigned index,
		const char *value)
{
	struct custom_property_storage *s = find_or_create(property, entity);

	return argz_property_seti(&s->argz, &s->argz_len, index, value);
}

static int custom_property_set(struct folder_property *property,
		struct folder_entity *entity, const char *value)
{
	return custom_property_seti(property, entity, 0, value);
}

static int custom_property_remove(struct rs_node *n)
{
	struct custom_property_storage *storage = to_custom_property_storage(n);

	memset(storage, 0, sizeof(*storage));
	free(storage);

	return 0;
}

static const struct rs_dll_vtable custom_property_storage_vtable = {
		.remove = custom_property_remove,
};

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

	rs_dll_init(&custom_property_storages, &custom_property_storage_vtable);
}

static __attribute__((destructor)) void custom_property_cleanup(void)
{
	ULOGD("%s", __func__);

}

