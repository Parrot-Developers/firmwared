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

static struct folder folders[FOLDERS_MAX];

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

/*
 * priority is needed so that folder implementations can make use of the folders
 * API
 */
__attribute__((constructor(FOLDERS_CONSTRUCTOR_PRIORITY)))
static void folders_init(void)
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
	if (ret < 0)
		ULOGE("load_words %s: %s", list_name, strerror(-ret));
	list_name = "adjectives";
	ret = load_words(resources_dir, list_name, &folders_adjectives);
	if (ret < 0)
		ULOGE("load_words %s: %s", list_name, strerror(-ret));
}

static int destroy_word(struct rs_node *node)
{
	struct word *word = ut_container_of(node, struct word, node);

	ut_string_free(&word->word);
	free(word);

	return 0;
}

__attribute__((destructor(FOLDERS_CONSTRUCTOR_PRIORITY)))
static void folders_cleanup(void)
{
	ULOGD("%s", __func__);

	rs_dll_remove_all_cb(&folders_names, destroy_word);
	rs_dll_remove_all_cb(&folders_adjectives, destroy_word);
}

static bool str_is_invalid(const char *str)
{
	return str == NULL || *str == '\0';
}

static bool folder_entity_ops_are_invalid(const struct folder_entity_ops *ops)
{
	return ops->drop == NULL || ops->get_info == NULL ||
			ops->sha1 == NULL || ops->can_drop == NULL;
}

static bool folder_is_invalid(const struct folder *folder)
{
	return folder == NULL || str_is_invalid(folder->name) ||
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

	return rs_dll_init(&(folders[i].entities), NULL);
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
		else if (strcmp(name, folders[i].name) == 0)
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
	struct folder *folder;
	struct rs_node *node;

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
		return -EINVAL;
	entity = to_entity(node);
	ut_string_free(&entity->name);

	return folder->ops.drop(entity, false);
}

int folder_store(const char *folder_name, struct folder_entity *entity)
{
	struct folder_entity *needle;
	struct folder *folder;

	folder = folder_find(folder_name);
	if (folder == NULL)
		return -ENOENT;
	entity->folder = folder;

	ut_string_free(&entity->name);
	entity->name = folder_request_friendly_name(folder);
	if (entity->name == NULL)
		return -errno;

	needle = find_entity(folder, folder_entity_get_sha1(entity));
	if (needle != NULL)
		return -EEXIST;

	rs_dll_push(&folder->entities, &entity->node);

	return 0;
}

char *folder_get_info(const char *folder_name,
		const char *entity_identifier)
{
	const struct folder *folder;
	struct folder_entity *entity;

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

	return folder->ops.get_info(entity);
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
	int old_errno;
	struct folder *folder = folders + FOLDERS_MAX;
	static char *list = NULL;
	char *tmp = NULL;

	/* the result is cached */
	if (list != NULL)
		return list;
	list = strdup("");

	if (list == NULL) {
		old_errno = errno;
		ULOGC("strdup: %m");
		errno = old_errno;
		return NULL;
	}

	while (folder-- > folders) {
		if (folder->name == NULL)
			continue;
		ret = asprintf(&tmp, "%s, %s", folder->name, list);
		if (ret < 0) {
			ULOGC("asprintf error");
			return NULL;
		}
		free(list);
		list = tmp;
	}
	if (list[0] != '\0')
		list[strlen(list) - 2] = '\0';

	return list;
}

const char *folder_entity_get_sha1(struct folder_entity *entity)
{
	errno = EINVAL;
	if (entity == NULL)
		return NULL;

	return entity->folder->ops.sha1(entity);
}

const char *folder_entity_get_name(const struct folder_entity *entity)
{
	errno = EINVAL;
	if (entity == NULL)
		return NULL;

	return entity->name;
}

int folder_unregister(const char *folder_name)
{
	struct folder *folder;
	struct folder *max = folders + FOLDERS_MAX - 1;
	int destroy_entity(struct rs_node *node)
	{
		struct folder_entity *entity = to_entity(node);

		ut_string_free(&entity->name);

		folder->ops.drop(entity, true);

		return 0;
	}

	folder = folder_find(folder_name);
	if (folder == NULL)
		return -ENOENT;

	rs_dll_remove_all_cb(&folder->entities, destroy_entity);

	for (; folder < max; folder++)
		*folder = *(folder + 1);
	memset(folder + 1, 0, sizeof(*folder)); /* NULL guard */

	return 0;
}
