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

int instances_init(void);
struct instance *instance_from_entity(struct folder_entity *entity);
struct folder_entity *instance_to_entity(struct instance *instance);
int instance_start(struct instance *instance);
int instance_kill(struct instance *instance, uint32_t killer_seqnum);
int instance_remount(struct instance *instance);
const char *instance_get_sha1(struct instance *instance);
const char *instance_get_name(const struct instance *instance);
void instance_delete(struct instance **instance, bool only_unregister);
void instances_cleanup(void);

#endif /* INSTANCES_H_ */
