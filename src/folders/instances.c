/**
 * @file instances.c
 * @brief
 *
 * @date Apr 21, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif /* _XOPEN_SOURCE */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <linux/limits.h>

#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>

#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <openssl/sha.h>

#define ULOG_TAG firmwared_instances
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_instances);

#include <ut_utils.h>
#include <ut_string.h>
#include <ut_file.h>
#include <ut_process.h>

#include "folders.h"
#include "instances.h"
#include "firmwares.h"
#include "utils.h"
#include "config.h"

struct instance {
	struct folder_entity entity;
	pid_t pid;
	enum instance_state state;
	char *firmware_path;
	/* caching of sha1 computation */
	char sha1[2 * SHA_DIGEST_LENGTH + 1];

	char *base_workspace;
	/* all 3 dirs must be subdirs of base_workspace dir */
	char *ro_mount_point;
	char *rw_dir;
	char *union_mount_point;

	/* pseudo-terminal passed to boxinit */
	char pts[PATH_MAX];
	int master;

	/* fields used for instance sha1 computation */
	char *firmware_sha1;
	time_t time;
};

#define to_instance(p) ut_container_of(p, struct instance, entity)

static struct folder instances_folder;

static int sha1(struct instance *instance,
		unsigned char hash[SHA_DIGEST_LENGTH])
{
	SHA_CTX ctx;

	SHA1_Init(&ctx);
	SHA1_Update(&ctx, instance->firmware_sha1,
			strlen(instance->firmware_sha1));
	SHA1_Update(&ctx, &instance->time, sizeof(instance->time));
	SHA1_Final(hash, &ctx);

	return 0;
}

static const char *compute_sha1(struct instance *instance)
{
	int ret;
	unsigned char hash[SHA_DIGEST_LENGTH];

	if (instance->sha1[0] == '\0') {
		ret = sha1(instance, hash);
		if (ret < 0) {
			errno = -ret;
			return NULL;
		}

		buffer_to_string(hash, SHA_DIGEST_LENGTH, instance->sha1);
	}

	return instance->sha1;
}

static const char *instance_sha1(struct folder_entity *entity)
{
	struct instance *instance = to_instance(entity);

	return compute_sha1(instance);
}

static void clean_pts(struct instance *instance)
{
	ut_file_fd_close(&instance->master);
	instance->pts[0] = '\0';
}

static void clean_paths(struct instance *instance)
{
	ut_string_free(&instance->base_workspace);
	ut_string_free(&instance->ro_mount_point);
	ut_string_free(&instance->rw_dir);
	ut_string_free(&instance->union_mount_point);
}

static int invoke_mount_helper(struct instance *instance, const char *action)
{
	return ut_process_vsystem("%s %s %s %s %s %s %s %s",
			config_get(CONFIG_MOUNT_HOOK),
			action,
			instance->base_workspace,
			instance->ro_mount_point,
			instance->rw_dir,
			instance->union_mount_point,
			instance->firmware_path,
			instance->firmware_sha1);
}

static void clean_mount_points(struct instance *instance)
{
	int ret;

	ret = invoke_mount_helper(instance, "clean");
	if (ret != 0)
		ULOGE("invoke_mount_helper clean returned %d", ret);
	clean_paths(instance);
}

static void instance_delete(struct instance **instance)
{
	struct instance *i;

	if (instance == NULL || *instance == NULL)
		return;
	i = *instance;

	clean_pts(i);
	clean_mount_points(i);

	ut_string_free(&i->firmware_sha1);
	ut_string_free(&i->firmware_path);
	free(i);
	*instance = NULL;
}

static bool instance_can_drop(struct folder_entity *entity)
{
	struct instance *instance = to_instance(entity);

	return instance->state == INSTANCE_READY;
}

static int instance_drop(struct folder_entity *entity)
{
	struct instance *instance = to_instance(entity);

	ULOGD("%s", __func__);

	instance_delete(&instance);

	return 0;
}

static char *instance_state_to_str(enum instance_state state)
{
	switch (state) {
	case INSTANCE_READY:
		return "ready";
	case INSTANCE_STARTED:
		return "started";
	case INSTANCE_STOPPING:
		return "stopping";
	default:
		return "(unknown)";
	}
}

static char *instance_get_info(struct folder_entity *entity)
{
	int ret;
	char *res;
	struct instance *instance = to_instance(entity);

	ret = asprintf(&res, "pid: %jd\n"
			"state: %s\n"
			"firmware_path: %s\n"
			"base_workspace: %s\n"
			"pts: %s\n"
			"firmware_sha1: %s\n"
			"time: %s\n",
			(intmax_t)instance->pid,
			instance_state_to_str(instance->state),
			instance->firmware_path,
			instance->base_workspace,
			instance->pts,
			instance->firmware_sha1,
			ctime(&instance->time));
	if (ret < 0) {
		ULOGE("asprintf error");
		errno = ENOMEM;
		return NULL;
	}

	return res;
}

struct folder_entity_ops instance_ops = {
		.sha1 = instance_sha1,
		.can_drop = instance_can_drop,
		.drop = instance_drop,
		.get_info = instance_get_info,
};

__attribute__((destructor(FOLDERS_CONSTRUCTOR_PRIORITY + 1)))
static void instances_cleanup(void)
{
	ULOGD("%s", __func__);

	/*
	 * instances destruction is managed by instance_drop, called on each
	 * instance by folder_unregister
	 */
	folder_unregister(INSTANCES_FOLDER_NAME);
}

__attribute__((constructor(FOLDERS_CONSTRUCTOR_PRIORITY + 1)))
static void instances_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	instances_folder.name = INSTANCES_FOLDER_NAME;
	memcpy(&instances_folder.ops, &instance_ops, sizeof(instance_ops));
	ret = folder_register(&instances_folder);
	if (ret < 0) {
		ULOGE("folder_register: %s", strerror(-ret));
		return;
	}
}

static int init_paths(struct instance *instance)
{
	int ret;

	ret = asprintf(&instance->base_workspace, "%s/%s",
			config_get(CONFIG_BASE_MOUNT_PATH),
			compute_sha1(instance));
	if (ret < 0) {
		instance->base_workspace = NULL;
		ULOGE("asprintf base_workspace error");
		ret = -ENOMEM;
		goto err;
	}
	ret = asprintf(&instance->ro_mount_point, "%s/ro",
			instance->base_workspace);
	if (ret < 0) {
		instance->ro_mount_point = NULL;
		ULOGE("asprintf ro_mount_point error");
		ret = -ENOMEM;
		goto err;
	}
	ret = asprintf(&instance->rw_dir, "%s/rw", instance->base_workspace);
	if (ret < 0) {
		instance->rw_dir = NULL;
		ULOGE("asprintf rw_dir error");
		ret = -ENOMEM;
		goto err;
	}
	ret = asprintf(&instance->union_mount_point, "%s/union",
			instance->base_workspace);
	if (ret < 0) {
		instance->union_mount_point = NULL;
		ULOGE("asprintf union_mount_point error");
		ret = -ENOMEM;
		goto err;
	}

	return 0;
err:
	clean_paths(instance);

	return ret;
}

static int init_mount_points(struct instance *instance)
{
	int ret;

	ret = init_paths(instance);
	if (ret < 0) {
		ULOGE("init_paths: %s", strerror(-ret));
		goto err;
	}
	ret = invoke_mount_helper(instance, "init");
	if (ret != 0) {
		ULOGE("invoke_mount_helper init returned %d", ret);
		ret = -ENOTRECOVERABLE;
		goto err;
	}

	return 0;
err:
	clean_mount_points(instance);

	return ret;
}

static int init_pts(struct instance *instance)
{
	int ret;

	instance->master = posix_openpt(O_RDWR | O_NOCTTY);
	if (instance->master < 0) {
		ret = -errno;
		ULOGE("posix_openpt: %m");
		goto err;
	}
	ret = grantpt(instance->master);
	if (ret < 0) {
		ret = -errno;
		ULOGE("grantpt: %m");
		goto err;
	}
	ret = unlockpt(instance->master);
	if (ret < 0) {
		ret = -errno;
		ULOGE("unlockpt: %m");
		goto err;
	}
	ret = ptsname_r(instance->master, instance->pts, PATH_MAX);
	if (ret < 0) {
		ret = -errno;
		ULOGE("ptsname_r: %m");
		goto err;
	}

	return 0;
err:
	clean_pts(instance);

	return ret;
}

struct instance *instance_new(struct firmware *firmware)
{
	int ret;
	struct instance *instance;

	instance = calloc(1, sizeof(*instance));
	if (instance == NULL)
		return NULL;
	instance->time = time(NULL);
	instance->pid = 0;
	instance->state = INSTANCE_READY;
	instance->master = -1;

	instance->firmware_sha1 = strdup(firmware_get_sha1(firmware));
	instance->firmware_path = strdup(firmware_get_path(firmware));
	if (instance->firmware_path == NULL || instance->firmware_sha1 == NULL)
		goto err;

	ret = init_mount_points(instance);
	if (ret < 0) {
		ULOGE("install_mount_points");
		errno = -ret;
		goto err;
	}

	ret = init_pts(instance);
	if (ret < 0) {
		ULOGE("install_mount_points");
		errno = -ret;
		goto err;
	}

	ret = folder_store(INSTANCES_FOLDER_NAME, &instance->entity);
	if (ret < 0) {
		ULOGE("folder_store: %s", strerror(-ret));
		goto err;
	}

	return instance;
err:
	instance_delete(&instance);

	return NULL;
}

const char *instance_get_sha1(const struct instance *instance)
{
	errno = EINVAL;
	if (instance == NULL)
		return NULL;

	return instance->sha1;
}

const char *instance_get_name(const struct instance *instance)
{
	errno = EINVAL;
	if (instance == NULL)
		return NULL;

	return instance->entity.name;
}
