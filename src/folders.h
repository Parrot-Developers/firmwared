/**
 * @file folders.h
 * @brief
 *
 * @date Apr 20, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef FOLDERS_H_
#define FOLDERS_H_
#include <stdbool.h>

#include <rs_dll.h>

#include "config.h"

/*
 * all the fields of the struct folder_entity are handled by the folders
 * module
 */
struct folder_entity {
	struct rs_node node;
	struct folder *folder;
	char *name;
};

struct folder;

struct folder_entity_ops {
	const char *(*sha1)(struct folder_entity *entity);
	bool (*can_drop)(struct folder_entity *entity);
	int (*drop)(struct folder_entity *entity, bool only_unregister);
};

struct folder_property {
	struct rs_node node;
	const char *name;
	/*
	 * allocates the string stored in value which must be freed after usage
	 */
	int (*get)(struct folder_entity *entity, char **value);
	int (*set)(struct folder_entity *entity, const char *value);

	/* array access functions */
	/*
	 * allocates the string stored in value which must be freed after usage
	 */
	int (*geti)(struct folder_entity *entity, int index, char **value);
	int (*seti)(struct folder_entity *entity, int index, const char *value);
};

struct folder {
	struct rs_dll entities;
	char *name;
	struct folder_entity_ops ops;
	struct rs_dll properties;
	struct folder_property name_property;
	struct folder_property sha1_property;
};

int folders_init(void);
int folder_register(const struct folder *folder);
struct folder *folder_find(const char *folder_name);
struct folder_entity *folder_next(const struct folder *folder,
		struct folder_entity *entity);
unsigned folder_get_count(const char *folder);
int folder_drop(const char *folder, struct folder_entity *entity);
/*
 * a folder_store call, transfers the ownership to the folder, except in case
 * of error, thus if anything bad happens, the entity will _not_ be destroyed
 */
int folder_store(const char *folder, struct folder_entity *entity);

char *folder_get_info(const char *folder, const char *entity_identifier);
struct folder_entity *folder_find_entity(const char *folder,
		const char *entity_identifier);
char *folder_list_properties(const char *folder_name);
const char *folders_list(void);
const char *folder_entity_get_sha1(struct folder_entity *entity);
/* only setter can be NULL */
int folder_register_property(const char *folder,
		struct folder_property *property);
/* string stored in value in output must be freed after usage */
int folder_entity_get_property(struct folder_entity *entity, const char *name,
		char **value);
int folder_entity_set_property(struct folder_entity *entity, const char *name,
		const char *value);
int folder_unregister(const char *folder);
void folders_cleanup(void);

#endif /* FOLDERS_H_ */
