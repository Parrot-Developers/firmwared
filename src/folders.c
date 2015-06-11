/**
 * @file folders.c
 * @brief
 *
 * @date Apr 20, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

#include <ut_utils.h>
#include <ut_string.h>
#include <ut_file.h>

#include "folders.h"
#include "config.h"

#define ULOG_TAG firmwared_folders
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_folders);

#define FOLDERS_MAX 3

#define to_entity(p) ut_container_of(p, struct folder_entity, node)
#define to_property(p) ut_container_of(p, struct folder_property, node)

static struct folder folders[FOLDERS_MAX];

static char *list;

static struct rs_dll folders_names;
static struct rs_dll folders_adjectives;

struct word {
	struct rs_node node;
	char *word;
};

static int store_word(struct rs_dll *list, const char *word)
{
	int ret;
	struct word *w;

	w = calloc(1, sizeof(*w));
	if (w == NULL) {
		ret = -errno;
		ULOGE("calloc: %s", strerror(-ret));
		return ret;
	}
	w->word = strdup(word);
	if (w->word == NULL) {
		ret = -errno;
		ULOGE("calloc: %s", strerror(-ret));
		free(w);
		return ret;
	}

	return rs_dll_enqueue(list, &w->node);
}

static int store_word_list(struct rs_dll *list, FILE *f)
{
	int ret;
	char *cret;
	char buf[0x100];
	char *word;

	rs_dll_init(list, NULL);

	while ((cret = fgets(buf, 0xFF, f)) != NULL) {
		buf[0xFF] = '\0';
		word = ut_string_strip(buf);
		if (strlen(word) != 0) {
			ret = store_word(list, word);
			if (ret < 0) {
				ULOGE("store_word: %s", strerror(-ret));
				return ret;
			}
		}
	}

	return 0;
}

static int load_words(const char *resources_dir, const char *list_name,
		struct rs_dll *list)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *path = NULL;
	FILE __attribute__((cleanup(ut_file_close))) *f = NULL;

	ret = asprintf(&path, "%s/%s", resources_dir, list_name);
	if (ret == -1) {
		path = NULL;
		ULOGE("asprintf error");
		exit(1);
	}
	f = fopen(path, "rbe");
	if (f == NULL) {
		ret = -errno;
		ULOGE("failure opening %s: %s", path, strerror(-ret));
		return ret;
	}
	ut_string_free(&path);

	return store_word_list(list, f);
}

static int destroy_word(struct rs_node *node)
{
	struct word *word = ut_container_of(node, struct word, node);

	ut_string_free(&word->word);
	free(word);

	return 0;
}

static bool folder_entity_ops_are_invalid(const struct folder_entity_ops *ops)
{
	return ops->drop == NULL || ops->sha1 == NULL || ops->can_drop == NULL;
}

static bool folder_is_invalid(const struct folder *folder)
{
	return folder == NULL || ut_string_is_invalid(folder->name) ||
			folder_entity_ops_are_invalid(&folder->ops);
}

static struct folder_entity *find_entity(const struct folder *folder,
		const char *identifier)
{
	struct folder_entity *entity = NULL;

	while ((entity = folder_next(folder, entity)))
		if (ut_string_match(identifier, folder_entity_get_sha1(entity))
				|| ut_string_match(identifier, entity->name))
			return entity;

	return NULL;
}

/* folder_entity_match_str_name */
static RS_NODE_MATCH_STR_MEMBER(folder_entity, name, node)

static const char *pick_random_word(struct rs_dll *word_list)
{
	int word_index;
	struct word *word = NULL;
	struct rs_node *word_node = NULL;

	word_index = (rand() % (rs_dll_get_count(word_list) - 1)) + 1;
	while (word_index--)
		word_node = rs_dll_next_from(word_list, word_node);

	word = ut_container_of(word_node, struct word, node);

	return word->word;
}

/* result must be freed with free */
static char *folder_request_friendly_name(struct folder *folder)
{
	int ret;
	const char *adjective;
	const char *name;
	char *friendly_name = NULL;
	unsigned max_names = rs_dll_get_count(&folders_names) *
				rs_dll_get_count(&folders_adjectives);

	errno = 0;
	if (rs_dll_get_count(&folder->entities) > max_names) {
		ULOGC("more than %u entities in folder %s, weird...", max_names,
				folder->name);
		errno = ENOMEM;
		return NULL;
	}

	do {
		adjective = pick_random_word(&folders_adjectives);
		name = pick_random_word(&folders_names);
		ut_string_free(&friendly_name); /* in case we loop */
		ret = asprintf(&friendly_name, "%s_%s", adjective, name);
		if (ret < 0) {
			ULOGE("asprintf error");
			errno = ENOMEM;
			return NULL;
		}
	} while (find_entity(folder, friendly_name) != NULL);

	return friendly_name;
}

static bool folder_can_drop(struct folder *folder, struct folder_entity *entity)
{
	return folder->ops.can_drop(entity);
}

static bool folder_property_is_array(struct folder_property *property)
{
	return property->geti != NULL;
}

static bool folder_property_is_invalid(struct folder_property *property)
{
	return property == NULL || ut_string_is_invalid(property->name) ||
			/* at least one getter is mandatory */
			(property->get == NULL && property->geti == NULL) ||
			/*
			 * if geti is defined, a get implementation is provided
			 * based on it
			 */
			(property->get != NULL && property->geti != NULL) ||
			/*
			 * if a setter is defined, the corresponding getter
			 * must be defined
			 */
			(property->set != NULL && property->get == NULL) ||
			(property->seti != NULL && property->geti == NULL);
}

/* folder_property_match_str_name */
static RS_NODE_MATCH_STR_MEMBER(folder_property, name, node)

static int folder_property_match_str_array_name(struct rs_node *node,
		const void *data)
{
	const char *name = (const char *)data;
	struct folder_property *property = to_property(node);
	if (name == NULL || property->name == NULL)
		return 0;

	if (ut_string_match(property->name, name))
		return true;

	/* test if the name could be an array access in the form name[...] */
	if (ut_string_match_prefix(name, property->name)) {
		if (name[strlen(property->name)] == '[' &&
				name[strlen(name) - 1] == ']')
			return true;
	}

	return false;
}

static int folder_entity_get_name(struct folder_entity *entity, char **value)
{
	if (entity == NULL || value == NULL)
		return -EINVAL;

	*value = strdup(entity->name);

	return *value == NULL ? -errno : 0;
}

static const struct folder_property name_property = {
		.name = "name",
		.get = folder_entity_get_name,
};

static int get_sha1(struct folder_entity *entity, char **value)
{
	const char *sha1;

	if (entity == NULL || value == NULL)
		return -EINVAL;

	sha1 = folder_entity_get_sha1(entity);
	if (sha1 == NULL)
		return -errno;

	*value = strdup(sha1);

	return *value == NULL ? -errno : 0;
}

static const struct folder_property sha1_property = {
		.name = "sha1",
		.get = get_sha1,
};

int folders_init(void)
{
	int ret;
	const char *resources_dir = config_get(CONFIG_RESOURCES_DIR);
	const char *list_name;
	time_t seed;

	ULOGD("%s", __func__);

	seed = time(NULL);
	srand(seed);
	ULOGI("random seed is %jd", (intmax_t)seed);

	list_name = "names";
	ret = load_words(resources_dir, list_name, &folders_names);
	if (ret < 0) {
		ULOGE("load_words %s: %s", list_name, strerror(-ret));
		return ret;
	}
	list_name = "adjectives";
	ret = load_words(resources_dir, list_name, &folders_adjectives);
	if (ret < 0) {
		ULOGE("load_words %s: %s", list_name, strerror(-ret));
		goto err;
	}

	return 0;
err:
	folders_cleanup();

	return ret;
}

static void print_folder_entities(struct rs_node *node)
{
	struct folder_entity *e = ut_container_of(node, typeof(*e), node);
	char __attribute__((cleanup(ut_string_free)))*info = NULL;

	info = folder_get_info(e->folder->name, folder_entity_get_sha1(e));
	ULOGN("%s", info);
}

static const struct rs_dll_vtable folder_vtable = {
	.print = print_folder_entities,
};

int folder_register(const struct folder *folder)
{
	const struct folder *needle;
	int i;

	if (folder_is_invalid(folder))
		return -EINVAL;

	/* folder name must be unique */
	needle = folder_find(folder->name);
	if (needle != NULL)
		return -EEXIST;

	/* find first free slot */
	for (i = 0; i < FOLDERS_MAX; i++)
		if (folders[i].name == NULL)
			break;

	if (i >= FOLDERS_MAX)
		return -ENOMEM;

	folders[i] = *folder;

	rs_dll_init(&(folders[i].properties), NULL);
	folders[i].name_property = name_property;
	folders[i].sha1_property = sha1_property;
	folder_register_property(folder->name, &folders[i].name_property);
	folder_register_property(folder->name, &folders[i].sha1_property);

	return rs_dll_init(&(folders[i].entities), &folder_vtable);
}

struct folder_entity *folder_next(const struct folder *folder,
		struct folder_entity *entity)
{
	errno = 0;

	if (folder == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return to_entity(rs_dll_next_from(&folder->entities, &entity->node));
}

struct folder *folder_find(const char *name)
{
	int i;

	errno = EINVAL;
	if (ut_string_is_invalid(name))
		return NULL;

	errno = ENOENT;
	for (i = 0; i < FOLDERS_MAX; i++)
		if (folders[i].name == NULL)
			return NULL;
		else if (ut_string_match(name, folders[i].name))
			return folders + i;

	return NULL;
}

unsigned folder_get_count(const char *folder_name)
{
	struct folder *folder;

	if (ut_string_is_invalid(folder_name))
		return (unsigned)-1;

	folder = folder_find(folder_name);
	if (folder == NULL)
		return -ENOENT;

	return rs_dll_get_count(&folder->entities);
}

int folder_drop(const char *folder_name, struct folder_entity *entity)
{
	int ret;
	struct folder *folder;
	struct rs_node *node;
	char *name;

	ULOGD("%s(%s, %p)", __func__, folder_name, entity);

	/* folder_name is checked in folder_find */
	if (entity == NULL)
		return -EINVAL;

	folder = folder_find(folder_name);
	if (folder == NULL)
		return -ENOENT;

	if (!folder_can_drop(folder, entity))
		return -EBUSY;

	node = rs_dll_remove_match(&folder->entities,
			folder_entity_match_str_name, entity->name);
	if (node == NULL)
		return -ESRCH;
	entity = to_entity(node);
	name = entity->name;

	ret = folder->ops.drop(entity, false);

	/* we have to free name after calling drop() in case it needs it */
	ut_string_free(&name);

	return ret;
}

int folder_store(const char *folder_name, struct folder_entity *entity)
{
	struct folder_entity *needle;
	struct folder *folder;

	ULOGD("%s(%s, %p)", __func__, folder_name, entity);

	folder = folder_find(folder_name);
	if (folder == NULL)
		return -ENOENT;
	entity->folder = folder;

	needle = find_entity(folder, folder_entity_get_sha1(entity));
	if (needle != NULL) {
		ULOGE("entity %s already exists",
				folder_entity_get_sha1(entity));
		return -EEXIST;
	}

	rs_dll_push(&folder->entities, &entity->node);

	/* must be done after stored, because it's _drop which frees it */
	ut_string_free(&entity->name);
	entity->name = folder_request_friendly_name(folder);
	if (entity->name == NULL)
		return -errno;

	return 0;
}

static int property_get(struct folder_property *property,
		struct folder_entity *entity, char **value)
{
	char *suffix;
	int i;
	int ret;

	if (!folder_property_is_array(property))
		return property->get(entity, value);

	for (i = 0; ; i++) {
		ret = property->geti(entity, i, &suffix);
		if (ret < 0) {
			ut_string_free(value);
			ULOGE("property->geti: %s", strerror(-ret));
			return ret;
		}
		if (ut_string_match(suffix, "nil")) {
			ut_string_free(&suffix);
			break;
		}
		ret = ut_string_append(value, "%s ", suffix);
		ut_string_free(&suffix);
		if (ret < 0) {
			ut_string_free(value);
			ULOGE("ut_string_append: %s", strerror(-ret));
			return ret;
		}
	}
	if (*value == NULL) {
		*value = strdup(" ");
		if (*value == NULL)
			return -errno;
	}
	(*value)[strlen(*value) - 1] = '\0';

	return 0;
}

char *folder_get_info(const char *folder_name,
		const char *entity_identifier)
{
	int ret;
	char *info = NULL;
	char *old_info = NULL;
	char *value = NULL;
	const struct folder *folder;
	struct folder_entity *entity;
	struct rs_node *node = NULL;
	struct folder_property *property;

	errno = 0;
	if (ut_string_is_invalid(folder_name) ||
			ut_string_is_invalid(entity_identifier)) {
		errno = EINVAL;
		return NULL;
	}

	folder = folder_find(folder_name);
	if (folder == NULL) {
		errno = ENOENT;
		return NULL;
	}

	entity = find_entity(folder, entity_identifier);
	if (entity == NULL)
		return NULL;

	while ((node = rs_dll_next_from(&folder->properties, node)) != NULL) {
		property = to_property(node);
		ret = property_get(property, entity, &value);
		if (ret < 0) {
			ut_string_free(&info);
			ULOGE("property_get: %s", strerror(-ret));
			errno = -ret;
			return NULL;
		}
		old_info = info;
		ret = asprintf(&info, "%s%s: %s\n", info ? info : "",
				property->name, value);
		ut_string_free(&value);
		ut_string_free(&old_info);
		if (ret < 0) {
			info = NULL;
			ULOGE("asprintf error");
			errno = -ENOMEM;
			return NULL;
		}
	}

	return info == NULL ? strdup("") : info;
}

struct folder_entity *folder_find_entity(const char *folder_name,
		const char *entity_identifier)
{
	struct folder *folder;
	struct folder_entity *entity;

	errno = ENOENT;
	if (ut_string_is_invalid(folder_name) ||
			ut_string_is_invalid(entity_identifier)) {
		errno = EINVAL;
		return NULL;
	}

	folder = folder_find(folder_name);
	if (folder == NULL)
		return NULL;

	entity = find_entity(folder, entity_identifier);
	if (entity == NULL)
		errno = ENOENT;

	return entity;
}

const char *folders_list(void)
{
	int ret;
	struct folder *folder = folders + FOLDERS_MAX;

	/* the result is cached */
	if (list != NULL)
		return list;

	while (folder-- > folders) {
		if (folder->name == NULL)
			continue;
		ret = ut_string_append(&list, "%s ", folder->name);
		if (ret < 0) {
			ULOGC("ut_string_append");
			errno = -ret;
			return NULL;
		}
	}
	if (list[0] != '\0')
		list[strlen(list) - 1] = '\0';

	return list;
}

char *folder_list_properties(const char *folder_name)
{
	int ret;
	struct folder *folder;
	char *properties_list = NULL;
	struct rs_node *node = NULL;
	struct folder_property *property;

	folder = folder_find(folder_name);
	if (folder == NULL)
		return NULL;

	while ((node = rs_dll_next_from(&folder->properties, node))) {
		property = to_property(node);
		if (folder_property_is_array(property))
			ret = ut_string_append(&properties_list, "%s[] ",
					property->name);
		else
			ret = ut_string_append(&properties_list, "%s ",
					property->name);
		if (ret < 0) {
			ULOGC("ut_string_append");
			errno = -ret;
			return NULL;
		}
	}
	if (properties_list[0] != '\0')
		properties_list[strlen(properties_list) - 1] = '\0';

	return properties_list;
}

const char *folder_entity_get_sha1(struct folder_entity *entity)
{
	errno = EINVAL;
	if (entity == NULL)
		return NULL;

	return entity->folder->ops.sha1(entity);
}

int folder_register_property(const char *folder_name,
		struct folder_property *property)
{
	struct folder *folder;

	if (ut_string_is_invalid(folder_name) ||
			folder_property_is_invalid(property))
		return -EINVAL;
	folder = folder_find(folder_name);
	if (folder == NULL)
		return -errno;

	return rs_dll_enqueue(&folder->properties, &property->node);
}

static int read_index(const char *name)
{
	long index;
	char *bracket;
	char *endptr;

	bracket = strchr(name, '[');
	if (bracket == NULL) {
		ULOGE("access to an array must be performed with [...]");
		return -1;
	}

	index = strtol(bracket + 1, &endptr, 0);
	if (*endptr != ']') {
		ULOGE("invalid array access near '%s'", bracket);
		return -1;
	}
	if (index >= INT_MAX) {
		ULOGE("index overflow: %ld", index);
		return -1;
	}

	return index;
}

int folder_entity_get_property(struct folder_entity *entity, const char *name,
		char **value)
{
	int ret = 0;
	struct rs_node *property_node;
	struct folder_property *property;
	bool is_array_access;
	int index;

	if (entity == NULL || ut_string_is_invalid(name) || value == NULL)
		return -EINVAL;

	property_node = rs_dll_find_match(&entity->folder->properties,
			folder_property_match_str_array_name, name);
	if (property_node == NULL) {
		ULOGE("property \"%s\" not found for folder %s", name,
				entity->folder->name);
		return -ESRCH;
	}
	property = to_property(property_node);
	is_array_access = name[strlen(name) - 1] == ']';
	if (is_array_access && !folder_property_is_array(property)) {
		ULOGE("non-array property accessed as an array one");
		return -EINVAL;
	}


	if (is_array_access) {
		index = read_index(name);
		if (index == -1)
			return -EINVAL;
		ret = property->geti(entity, index, value);
		if (ret < 0) {
			ULOGE("property->geti: %s", strerror(-ret));
			return ret;
		}
		if (*value == NULL) {
			*value = strdup("nil");
			if (*value == NULL)
				return -errno;
		}
	} else {
		ret = property_get(property, entity, value);
		if (ret < 0) {
			ULOGE("property_get: %s", strerror(-ret));
			return ret;
		}
	}

	return *value == NULL ? -errno : 0;
}

int folder_entity_set_property(struct folder_entity *entity, const char *name,
		const char *value)
{
	int ret = 0;
	int index;
	bool is_array_access;
	struct rs_node *property_node;
	struct folder_property *property;

	if (entity == NULL || ut_string_is_invalid(name) ||
			ut_string_is_invalid(value))
		return -EINVAL;

	property_node = rs_dll_find_match(&entity->folder->properties,
			folder_property_match_str_array_name, name);
	if (property_node == NULL) {
		ULOGE("property \"%s\" not found for folder %s", name,
				entity->folder->name);
		return -ESRCH;
	}
	property = to_property(property_node);
	is_array_access = name[strlen(name) - 1] == ']';
	if (is_array_access != folder_property_is_array(property)) {
		if (is_array_access)
			ULOGE("non-array property accessed as an array one");
		else
			ULOGE("array property accessed as a non-array one");
		return -EINVAL;
	}

	if (is_array_access) {
		if (property->seti == NULL) {
			ULOGE("property %s.%s[] is read-only",
					entity->folder->name, name);
			return -EPERM;
		}
		index = read_index(name);
		if (index == -1)
			return -EINVAL;
		ret = property->seti(entity, index, value);
		if (ret < 0) {
			ULOGE("property->seti: %s", strerror(-ret));
			return ret;
		}
	} else {
		if (property->set == NULL) {
			ULOGE("property %s.%s is read-only",
					entity->folder->name, name);
			return -EPERM;
		}
		ret = property->set(entity, value);
		if (ret < 0) {
			ULOGE("property->set: %s", strerror(-ret));
			return ret;
		}
	}

	return 0;
}

int folder_unregister(const char *folder_name)
{
	struct folder *folder;
	struct folder *max = folders + FOLDERS_MAX - 1;
	int destroy_entity(struct rs_node *node)
	{
		char *name;
		struct folder_entity *entity = to_entity(node);

		name = entity->name;

		folder->ops.drop(entity, true);

		/* we have to free name after calling drop() in case it needs it */
		ut_string_free(&name);

		return 0;
	}

	folder = folder_find(folder_name);
	if (folder == NULL)
		return -ENOENT;

	rs_dll_remove_all_cb(&folder->entities, destroy_entity);
	rs_dll_remove_all(&folder->properties);

	for (; folder < max; folder++)
		*folder = *(folder + 1);
	memset(max - 1, 0, sizeof(*folder)); /* NULL guard */

	return 0;
}

void folders_cleanup(void)
{
	ULOGD("%s", __func__);

	ut_string_free(&list);
	rs_dll_remove_all_cb(&folders_names, destroy_word);
	rs_dll_remove_all_cb(&folders_adjectives, destroy_word);
}
