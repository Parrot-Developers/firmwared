/**
 * @file instances.h
 * @brief
 *
 * @date Apr 21, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef INSTANCES_H_
#define INSTANCES_H_

#include "firmwared.h"
#include "firmwares.h"

struct instance;

#define INSTANCES_FOLDER_NAME "instances"

enum instance_state {
	INSTANCE_READY,
	INSTANCE_STARTED,
	INSTANCE_STOPPING,
};

int instances_init(void);
/*
 * a newly created instance is automatically stored in the instance folder and
 * will be destroyed automatically on a drop operation
 */
struct instance *instance_new(struct firmwared *firmwared, const char *path,
		const char *sha1);
struct instance *instance_from_entity(struct folder_entity *entity);
int instance_start(struct instance *instance);
int instance_kill(struct instance *instance, uint32_t killer_msgid);
const char *instance_get_sha1(struct instance *instance);
const char *instance_get_name(const struct instance *instance);
void instances_cleanup(void);

#endif /* INSTANCES_H_ */
