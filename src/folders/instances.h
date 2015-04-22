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

#include "firmwares.h"

struct instance;

#define INSTANCES_FOLDER_NAME "instances"

enum instance_state {
	INSTANCE_READY,
	INSTANCE_STARTED,
	INSTANCE_STOPPING,
};

/*
 * a newly created instance is automatically stored in the instance folder and
 * will be destroyed automatically on a drop operation
 */
struct instance *instance_new(struct firmware *firmware);
const char *instance_get_sha1(const struct instance *instance);
const char *instance_get_name(const struct instance *instance);

#endif /* INSTANCES_H_ */