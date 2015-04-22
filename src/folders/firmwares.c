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

#define FOLDER_NAME "firmwares"

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

static int firmware_store(struct folder_entity *entity)
{
	/* not used for this folder */

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
		.store = firmware_store,
		.get_info = firmware_get_info,
};

static int pattern_filter(const struct dirent *d)
{
	return fnmatch(FIRMWARE_MATCHING_PATTERN, d->d_name, 0) == 0;
}

static struct firmware *firmware_new(const char *repository_path,
		const char *path)
{
	int ret;
	struct firmware *firmware;
	const char *firmware_repository_path =
			config_get(CONFIG_FIRMWARE_REPOSITORY);

	firmware = calloc(1, sizeof(*firmware));
	if (firmware == NULL)
		return NULL;

	ret = asprintf(&firmware->path, "%s/%s", firmware_repository_path,
			path);
	if (ret == -1) {
		ULOGE("asprintf error");
		free(firmware);
		errno = ENOMEM;
		return NULL;
	}

	return firmware;
}

static int index_firmwares(void)
{
	int ret;
	int n;
	struct dirent **namelist;
	struct firmware *f;
	const char *firmware_repository_path =
			config_get(CONFIG_FIRMWARE_REPOSITORY);

	ULOGI("indexing "FOLDER_NAME);

	n = scandir(firmware_repository_path, &namelist, pattern_filter, NULL);
	if (n == -1) {
		ret = -errno;
		ULOGE("%s scandir: %m", __func__);
		return ret;
	}

	while (n--) {
		ULOGD("indexing firmware %s", namelist[n]->d_name);
		f = firmware_new(firmware_repository_path, namelist[n]->d_name);
		if (f == NULL) {
			ret = -errno;
			ULOGE("firmware_new: %m");
			return ret;
		}

		ret = folder_store(FOLDER_NAME, &f->entity);
		if (ret < 0) {
			firmware_delete(&f);
			ULOGE("folder_store: %s", strerror(-ret));
			return ret;
		}
	}

	ULOGI("done indexing "FOLDER_NAME);

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
	folder_unregister(FOLDER_NAME);
}

__attribute__((constructor(FOLDERS_CONSTRUCTOR_PRIORITY + 1)))
static void firmwares_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	firmware_folder.name = FOLDER_NAME;
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

const char *firmware_get_path(struct firmware *firmware)
{
	errno = EINVAL;
	if (firmware == NULL)
		return NULL;

	return firmware->path;
}

const char *firmware_get_sha1(const struct firmware *firmware)
{
	if (firmware == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return firmware->entity.sha1;
}

