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

#define FOLDER_NAME "firmware"

#define FIRMWARE_REPOSITORY_PATH "/usr/share/firmwared/firmwares/"

#define FIRMWARE_REPOSITORY_PATH_ENV "FIRMWARE_REPOSITORY_PATH"

#ifndef FIRMWARE_MATCHING_PATTERN
#define FIRMWARE_MATCHING_PATTERN "*.firmware"
#endif

#define BUF_SIZE 0x200

static char *firmware_repository_path;

struct firmware {
	struct folder_entity entity;
	char *path;
};

#define to_firmware(p) ut_container_of(p, struct firmware, entity)

static struct folder firmware_folder;

static char *buffer_to_string(const unsigned char *src, size_t len, char *dst)
{
	static const char lut[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8',
		'9', 'a', 'b', 'c', 'd', 'e', 'f'};

	/* not necessary, but more explicit */
	dst[2 * len] = '\0';

	while (len--) {
		dst[2 * len] = lut[(src[len] & 0xF0) >> 4];
		dst[2 * len + 1] = lut[src[len] & 0x0F];
	}

	return dst;
}

static char *firmware_sha1(struct folder_entity *entity)
{
	int old_errno;
	size_t count;
	struct firmware *firmware = to_firmware(entity);
	unsigned char hash[SHA_DIGEST_LENGTH];
	SHA_CTX ctx;
	FILE __attribute__((cleanup(ut_file_close))) *f = NULL;
	char buf[BUF_SIZE];
	char *res;

	f = fopen(firmware->path, "rbe");
	if (f == NULL) {
		old_errno = errno;
		ULOGE("%s: fopen : %m", __func__);
		errno = old_errno;
		return NULL;
	}

	SHA1_Init(&ctx);
	do {
		count = fread(buf, BUF_SIZE, 1, f);
		if (count != 0)
			SHA1_Update(&ctx, buf, count);
	} while (count == BUF_SIZE);
	SHA1_Final(hash, &ctx);
	if (ferror(f)) {
		ULOGE("error reading %s for sha1 computation", firmware->path);
		errno = EIO;
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
/*	struct firmware *firmware = to_firmware(entity);*/

	ULOGC("%s: STUB !!!", __func__); // TODO

	return -ENOSYS; // TODO
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

	ULOGD("%s", __func__);

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

	return 0;
}

static __attribute__((constructor(102))) void firmwares_cleanup(void)
{
	/*
	 * firmwares destruction is managed by firmware_drop, called on each
	 * firmware by folder_unregister
	 */
	folder_unregister(FOLDER_NAME);
}

static __attribute__((constructor(102))) void firmwares_init(void)
{
	int ret;

	firmware_repository_path = getenv(FIRMWARE_REPOSITORY_PATH_ENV);
	if (firmware_repository_path == NULL)
		firmware_repository_path = FIRMWARE_REPOSITORY_PATH;

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

