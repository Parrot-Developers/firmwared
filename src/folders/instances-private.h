/**
 * @file instances-private.h
 * @brief
 *
 * @date May 29, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef INSTANCES_PRIVATE_H_
#define INSTANCES_PRIVATE_H_
#include <sys/types.h>

#include <stdint.h>
#include <time.h>

#include <openssl/sha.h>

#include <uv.h>

#include <ut_process.h>

#include <ptspair.h>

#include "../folders.h"

enum instance_state {
	INSTANCE_READY,
	INSTANCE_STARTED,
	INSTANCE_STOPPING,
};

struct instance {
	/* runtime unique id */
	uint8_t id;

	struct folder_entity entity;
	pid_t pid;
	int pidfd;
	uv_poll_t pidfd_handle;
	enum instance_state state;
	char *firmware_path;
	/* caching of sha1 computation */
	char sha1[2 * SHA_DIGEST_LENGTH + 1];
	char *info;
	uint32_t killer_msgid;

	/* synchronization between monitor and pid 1 */
	struct ut_process_sync sync;

	char *base_workspace;
	/* all 3 dirs must be subdirs of base_workspace dir */
	char *ro_mount_point;
	char *rw_dir;
	char *union_mount_point;

	/* foo is the external pts, bar will be passed to the pid 1 */
	struct ptspair ptspair;
	uv_poll_t ptspair_handle;

	/* fields used for instance sha1 computation */
	char *firmware_sha1;
	time_t time;

	/* run-time configurable properties */
	char *interface;
};

#define to_instance(p) ut_container_of(p, struct instance, entity)

char *instance_state_to_str(enum instance_state state);

#endif /* INSTANCES_PRIVATE_H_ */
