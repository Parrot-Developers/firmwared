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

#include <io_src_evt.h>

#include <ut_process.h>

#include <ptspair.h>

#include "../folders.h"

enum instance_state {
	INSTANCE_READY,
	INSTANCE_STARTED,
	INSTANCE_STOPPING,
};

struct instance {
	struct folder_entity entity;

	enum instance_state state;
	/* caching of sha1 computation */
	char sha1[2 * SHA_DIGEST_LENGTH + 1];
	char *info;
	uint32_t killer_seqnum;

	/* synchronization between monitor and pid 1 */
	struct ut_process_sync sync;

	/* all 3 dirs must be subdirs of base_workspace dir */
	char *ro_mount_point;
	char *rw_dir;
	char *union_mount_point;

	/* for monitoring the monitor process */
	struct io_src_evt monitor_evt;
	pid_t pid;

	/* foo is the external pts, bar will be passed to the pid 1 */
	struct ptspair ptspair;
	struct io_src ptspair_src;


	/* run-time configurable properties */
	char *interface;
	char *stolen_interface;

	char *command_line;
	size_t command_line_len;

	/* all the remaining fields are used for instance sha1 computation */
	char *firmware_path;
	time_t time;
	/* runtime unique id */
	uint8_t id;
};

#define to_instance(p) ut_container_of(p, struct instance, entity)

char *instance_state_to_str(enum instance_state state);

#endif /* INSTANCES_PRIVATE_H_ */
