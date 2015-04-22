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

enum instance_state {
	INSTANCE_READY,
	INSTANCE_STARTED,
	INSTANCE_STOPPING,
};

struct instance *instance_new(struct firmware *firmware);
void instance_delete(struct instance **instance);

#endif /* INSTANCES_H_ */
