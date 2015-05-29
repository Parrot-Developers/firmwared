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
#include <unistd.h>
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
	char sha1[2 * SHA_DIGEST_LENGTH + 1];
};

#define to_firmware(p) ut_container_of(p, struct firmware, entity)

static struct folder firmware_folder;

static int sha1(struct firmware *firmware,
		unsigned char hash[SHA_DIGEST_LENGTH])
{
	int ret;
	size_t count;
	SHA_CTX ctx;
	FILE __attribute__((cleanup(ut_file_close))) *f = NULL;
	char buf[BUF_SIZE] = {0};

	f = fopen(firmware->path, "rbe");
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
		ULOGE("error reading %s for sha1 computation", firmware->path);
		return -EIO;
	}

	return 0;
}

static const char *compute_sha1(struct firmware *firmware)
{
	int ret;
	unsigned char hash[SHA_DIGEST_LENGTH];

	if (firmware->sha1[0] == '\0') {
		ret = sha1(firmware, hash);
		if (ret < 0) {
			errno = -ret;
			return NULL;
		}

		buffer_to_string(hash, SHA_DIGEST_LENGTH, firmware->sha1);
	}

	return firmware->sha1;
}

static const char *firmware_sha1(struct folder_entity *entity)
{
	struct firmware *firmware = to_firmware(entity);

	return compute_sha1(firmware);
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

static bool firmware_can_drop(struct folder_entity *entity)
{
	return true;
}

static int firmware_drop(struct folder_entity *entity, bool only_unregister)
{
	struct firmware *firmware = to_firmware(entity);

	ULOGD("%s", __func__);

	if (!only_unregister)
		unlink(firmware->path);
	firmware_delete(&firmware);

	return 0;
}

static char *firmware_get_info(const struct folder_entity *entity)
{
	struct firmware *firmware = to_firmware(entity);

	return strdup(firmware->path);
}

struct folder_entity_ops firmware_ops = {
		.sha1 = firmware_sha1,
		.can_drop = firmware_can_drop,
		.drop = firmware_drop,
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
	const char *sha1;
	struct firmware *firmware;
	const char *firmware_repository_path =
			config_get(CONFIG_REPOSITORY_PATH);

	ULOGD("indexing firmware %s", path);

	firmware = calloc(1, sizeof(*firmware));
	if (firmware == NULL)
		return NULL;

	ret = asprintf(&firmware->path, "%s/%s", firmware_repository_path,
			path);
	if (ret == -1) {
		ULOGE("asprintf error");
		errno = -ENOMEM;
		goto err;
	}

	/* force sha1 computation while in parallel section */
	sha1 = compute_sha1(firmware);
	if (sha1 == NULL)
		goto err;

	ULOGD("indexing firmware %s done", path);

	return firmware;
err:
	firmware_delete(&firmware);

	return NULL;
}

static void free_namelist(struct dirent ***namelist)
{
	if (namelist == NULL || *namelist == NULL)
		return;
	free(*namelist);
	*namelist = NULL;
}

static void free_firmwares(struct firmware ***firmwares)
{
	if (firmwares == NULL || *firmwares == NULL)
		return;
	free(*firmwares);
	*firmwares = NULL;
}

static int index_firmwares(void)
{
	int i;
	int ret;
	int res = 0;
	int n;
	struct dirent __attribute__((cleanup(free_namelist))) **namelist = NULL;
	const char *repository = config_get(CONFIG_REPOSITORY_PATH);
	struct firmware __attribute__((cleanup(free_firmwares))) **firmwares;

	ULOGI("indexing "FIRMWARES_FOLDER_NAME);

	n = scandir(repository, &namelist, pattern_filter, NULL);
	if (n == -1) {
		ret = -errno;
		ULOGE("%s scandir: %m", __func__);
		return ret;
	}
	firmwares = calloc(n, sizeof(*firmwares));
	if (firmwares == NULL)
		return -errno;

	{
#pragma omp parallel for
		for (i = 0; i < n; i++) {
			firmwares[i] = firmware_new(repository,
					namelist[i]->d_name);
			free(namelist[i]);
		}
	}

	while (n--) {
		if (firmwares[n] == NULL) {
			res = -ENOMEM;
			continue;
		}

		ret = folder_store(FIRMWARES_FOLDER_NAME,
				&(firmwares[n]->entity));
		if (ret < 0) {
			ULOGE("folder_store: %s", strerror(-ret));
			res = ret;
			/*
			 * we mustn't go out, we have to unstore and destroy
			 * every firmware in case of error
			 */
		}
	}

	ULOGI("done indexing "FIRMWARES_FOLDER_NAME);

	return res;
}

static int get_path(struct folder_entity *entity, char **value)
{
	struct firmware *firmware;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	firmware = to_firmware(entity);

	*value = strdup(firmware->path);

	return *value == NULL ? -errno : 0;
}

static struct folder_property path_property = {
		.name = "path",
		.get = get_path,
};

int firmwares_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	firmware_folder.name = FIRMWARES_FOLDER_NAME;
	memcpy(&firmware_folder.ops, &firmware_ops, sizeof(firmware_ops));
	ret = folder_register(&firmware_folder);
	if (ret < 0) {
		ULOGE("folder_register: %s", strerror(-ret));
		return ret;
	}
	folder_register_property(FIRMWARES_FOLDER_NAME, &path_property);

	ret = index_firmwares();
	if (ret < 0) {
		ULOGE("index_firmwares: %s", strerror(-ret));
		firmwares_cleanup();
	}

	return ret;
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

	return firmware->sha1;
}

const char *firmware_get_name(const struct firmware *firmware)
{
	errno = EINVAL;
	if (firmware == NULL)
		return NULL;

	return firmware->entity.name;
}

void firmwares_cleanup(void)
{
	ULOGD("%s", __func__);

	/*
	 * firmwares destruction is managed by firmware_drop, called on each
	 * firmware by folder_unregister
	 */
	folder_unregister(FIRMWARES_FOLDER_NAME);
}
