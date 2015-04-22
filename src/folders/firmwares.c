/**
 * @file firmwares.c
 * @brief
 *
 * @date Apr 20, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <dirent.h>
#include <fnmatch.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <openssl/sha.h>

#define ULOG_TAG firmwared_firmwares
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_firmwares);

#include <ut_utils.h>
#include <ut_string.h>
#include <ut_file.h>

#include "folders.h"
#include "firmwares.h"
#include "utils.h"
#include "config.h"

#ifndef FIRMWARE_MATCHING_PATTERN
#define FIRMWARE_MATCHING_PATTERN "*.firmware"
#endif

#define BUF_SIZE 0x200

struct firmware {
	struct folder_entity entity;
	char *path;
};

#define to_firmware(p) ut_container_of(p, struct firmware, entity)

static struct folder firmware_folder;

static int sha1(const char *path, unsigned char hash[SHA_DIGEST_LENGTH])
{
	int ret;
	size_t count;
	SHA_CTX ctx;
	FILE __attribute__((cleanup(ut_file_close))) *f = NULL;
	char buf[BUF_SIZE] = {0};

	f = fopen(path, "rbe");
	if (f == NULL) {
		ret = -errno;
		ULOGE("%s: fopen : %m", __func__);
		return ret;
	}

	SHA1_Init(&ctx);
	do {
		count = fread(buf, 1, BUF_SIZE, f);
		if (count != 0)
			SHA1_Update(&ctx, buf, count);
	} while (count == BUF_SIZE);
	SHA1_Final(hash, &ctx);
	if (ferror(f)) {
		ULOGE("error reading %s for sha1 computation", path);
		return -EIO;
	}

	return 0;
}

static char *firmware_sha1(struct folder_entity *entity)
{
	int ret;
	struct firmware *firmware = to_firmware(entity);
	unsigned char hash[SHA_DIGEST_LENGTH];
	char *res;

	ret = sha1(firmware->path, hash);
	if (ret < 0) {
		errno = -ret;
		return NULL;
	}

	res = calloc(2 * SHA_DIGEST_LENGTH + 1, sizeof(*res));
	if (res == NULL)
		return NULL;

	return buffer_to_string(hash, SHA_DIGEST_LENGTH, res);
}

static void firmware_delete(struct firmware **firmware)
{
	struct firmware *f;

	if (firmware == NULL || *firmware == NULL)
		return;
	f = *firmware;

	ut_string_free(&f->path);
	free(f);
	*firmware = NULL;
}

static int firmware_drop(struct folder_entity *entity)
{
	struct firmware *firmware = to_firmware(entity);

	ULOGD("%s", __func__);

	firmware_delete(&firmware);

	return 0;
}

static char *firmware_get_info(struct folder_entity *entity)
{
	struct firmware *firmware = to_firmware(entity);

	return strdup(firmware->path);
}

struct folder_entity_ops firmware_ops = {
		.sha1 = firmware_sha1,
		.drop = firmware_drop,
		.get_info = firmware_get_info,
};

static int pattern_filter(const struct dirent *d)
{
	return fnmatch(FIRMWARE_MATCHING_PATTERN, d->d_name, 0) == 0;
}

static int firmware_new(const char *repository_path, const char *path)
{
	int ret;
	struct firmware *firmware;
	const char *firmware_repository_path =
			config_get(CONFIG_FIRMWARE_REPOSITORY);

	ULOGD("indexing firmware %s", path);

	firmware = calloc(1, sizeof(*firmware));
	if (firmware == NULL)
		return -errno;

	ret = asprintf(&firmware->path, "%s/%s", firmware_repository_path,
			path);
	if (ret == -1) {
		ULOGE("asprintf error");
		ret = -ENOMEM;
		goto err;
	}

	ret = folder_store(FIRMWARES_FOLDER_NAME, &firmware->entity);
	if (ret < 0) {
		ULOGE("folder_store: %s", strerror(-ret));
		goto err;
	}

	return 0;
err:
	firmware_delete(&firmware);

	return ret;
}

static int index_firmwares(void)
{
	int ret;
	int n;
	struct dirent **namelist;
	const char *repository = config_get(CONFIG_FIRMWARE_REPOSITORY);

	ULOGI("indexing "FIRMWARES_FOLDER_NAME);

	n = scandir(repository, &namelist, pattern_filter, NULL);
	if (n == -1) {
		ret = -errno;
		ULOGE("%s scandir: %m", __func__);
		return ret;
	}

	while (n--) {
		ret = firmware_new(repository, namelist[n]->d_name);
		if (ret < 0) {
			ULOGE("firmware_new: %s", strerror(-ret));
			return ret;
		}
	}

	ULOGI("done indexing "FIRMWARES_FOLDER_NAME);

	return 0;
}

__attribute__((destructor(FOLDERS_CONSTRUCTOR_PRIORITY + 1)))
static void firmwares_cleanup(void)
{
	ULOGD("%s", __func__);

	/*
	 * firmwares destruction is managed by firmware_drop, called on each
	 * firmware by folder_unregister
	 */
	folder_unregister(FIRMWARES_FOLDER_NAME);
}

__attribute__((constructor(FOLDERS_CONSTRUCTOR_PRIORITY + 1)))
static void firmwares_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	firmware_folder.name = FIRMWARES_FOLDER_NAME;
	memcpy(&firmware_folder.ops, &firmware_ops, sizeof(firmware_ops));
	ret = folder_register(&firmware_folder);
	if (ret < 0) {
		ULOGE("folder_register: %s", strerror(-ret));
		return;
	}

	ret = index_firmwares();
	if (ret < 0) {
		ULOGE("index_firmwares: %s", strerror(-ret));
		firmwares_cleanup();
	}
}

struct firmware *firmware_from_entity(struct folder_entity *entity)
{
	errno = -EINVAL;
	if (entity == NULL)
		return NULL;

	return to_firmware(entity);
}

const char *firmware_get_path(struct firmware *firmware)
{
	errno = EINVAL;
	if (firmware == NULL)
		return NULL;

	return firmware->path;
}

const char *firmware_get_sha1(const struct firmware *firmware)
{
	errno = EINVAL;
	if (firmware == NULL)
		return NULL;

	return firmware->entity.sha1;
}

const char *firmware_get_name(const struct firmware *firmware)
{
	errno = EINVAL;
	if (firmware == NULL)
		return NULL;

	return firmware->entity.name;
}
