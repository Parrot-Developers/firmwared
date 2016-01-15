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
#include <sys/stat.h>
#include <sys/types.h>

#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <argz.h>
#include <envz.h>

#include <openssl/sha.h>

#define ULOG_TAG firmwared_firmwares
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_firmwares);

#include <io_process.h>

#include <ut_utils.h>
#include <ut_string.h>
#include <ut_file.h>
#include <ut_process.h>

#include <fwd.h>

#include "preparation.h"
#include "folders.h"
#include "firmwares.h"
#include "log.h"
#include "utils.h"
#include "config.h"
#include "process.h"
#include "firmwares-private.h"
#include "properties/firmware_properties.h"

#ifndef FIRMWARE_SUFFIX
#define FIRMWARE_SUFFIX ".firmware"
#endif

#ifndef FIRMWARE_MATCHING_PATTERN
#define FIRMWARE_MATCHING_PATTERN "*"FIRMWARE_SUFFIX
#endif

#ifndef PREPARATION_TIMEOUT
#define PREPARATION_TIMEOUT 10000
#endif /* PREPARATION_TIMEOUT */

#ifndef PREPARATION_TIMEOUT_SIGNAL
#define PREPARATION_TIMEOUT_SIGNAL SIGKILL
#endif /* PREPARATION_TIMEOUT_SIGNAL */

#define BUF_SIZE 0x200

struct firmware_preparation {
	struct preparation preparation;
	struct io_process process;
	char *destination_file;
};

static struct folder firmware_folder;

static int sha1(struct firmware *firmware,
		unsigned char hash[SHA_DIGEST_LENGTH])
{
	int ret;
	size_t count;
	SHA_CTX ctx;
	FILE __attribute__((cleanup(ut_file_close))) *f = NULL;
	char buf[BUF_SIZE] = {0};

	SHA1_Init(&ctx);
	if (ut_file_is_dir(firmware->path)) {
		SHA1_Update(&ctx, firmware->path, strlen(firmware->path));
	} else {
		f = fopen(firmware->path, "rbe");
		if (f == NULL) {
			ret = -errno;
			ULOGE("%s: fopen(%s) : %m", firmware->path, __func__);
			return ret;
		}

		do {
			count = fread(buf, 1, BUF_SIZE, f);
			if (count != 0)
				SHA1_Update(&ctx, buf, count);
		} while (count == BUF_SIZE);
		if (ferror(f)) {
			ULOGE("error reading %s for sha1 computation",
					firmware->path);
			return -EIO;
		}
	}
	SHA1_Final(hash, &ctx);

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

static void unmount_firmware(struct firmware *firmware)
{
	int ret;
	struct io_process process;
	const char *mount_dir =
			folder_entity_get_base_workspace(&firmware->entity);

	ret = io_process_init_prepare_launch_and_wait(&process,
			&process_default_parameters, NULL, "/bin/umount",
			mount_dir, NULL);
	if (ret < 0)
		ULOGE("umount %s failed: %m", mount_dir);
	else if (process.status != 0)
		ULOGE("umount %s exited with status %d", mount_dir,
				process.status);
}

static void firmware_delete(struct firmware **firmware)
{
	struct firmware *f;

	if (firmware == NULL || *firmware == NULL)
		return;
	f = *firmware;

	unmount_firmware(f);
	ut_string_free(&f->path);
	ut_string_free(&f->uuid);
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

static struct firmware *get_from_uuid(const char *uuid)
{
	struct folder *folder = folder_find(FIRMWARES_FOLDER_NAME);
	struct folder_entity *entity = NULL;
	struct firmware *firmware;

	for (entity = folder_next(folder, entity);
			entity != NULL;
			entity = folder_next(folder, entity)) {
		firmware = firmware_from_entity(entity);
		if (ut_string_match(uuid, firmware_get_uuid(firmware)))
			return firmware;
	}

	return NULL;
}

static struct firmware *get_from_path(const char *path)
{
	struct folder *folder = folder_find(FIRMWARES_FOLDER_NAME);
	struct folder_entity *entity = NULL;
	struct firmware *firmware;

	for (entity = folder_next(folder, entity);
			entity != NULL;
			entity = folder_next(folder, entity)) {
		firmware = firmware_from_entity(entity);
		if (ut_string_match(path, firmware_get_path(firmware)))
			return firmware;
	}

	return NULL;
}

static bool uuid_already_registered(const char *uuid)
{
	return get_from_uuid(uuid) != NULL;
}

static void preparation_progress_sep_cb(struct io_src_sep *sep, char *chunk,
		unsigned len)
{
	int ret;
	struct firmware_preparation *firmware_preparation;
	struct preparation *preparation;
	struct io_process *process;

	if (len == 0)
		return;

	process = ut_container_of(sep, struct io_process, stdout_src);
	firmware_preparation = ut_container_of(process,
			struct firmware_preparation, process);
	preparation = &firmware_preparation->preparation;

	chunk[len] = '\0';
	ut_string_rstrip(chunk);
	ULOGD("%s "FIRMWARES_FOLDER_NAME" preparation %s",
			preparation->identification_string, chunk);

	/*
	 * rearm the "watch dog", we don't want to abort a working dl because it
	 * took too long
	 */
	ret = io_process_set_timeout(process, PREPARATION_TIMEOUT,
			PREPARATION_TIMEOUT_SIGNAL);
	if (ret < 0)
		ULOGW("resetting firmware preparation timeout failed: %s",
				strerror(-ret));

	if (ut_string_match_prefix(chunk, "destination_file=")) {
		firmware_preparation->destination_file = strdup(chunk + 17);
		/*
		 * here the hook is stuck in a sleep, waiting for us to say it
		 * it can nicely die
		 */
		io_process_signal(process, SIGUSR1);
	} else {
		ret = firmwared_notify(FWD_ANSWER_PREPARE_PROGRESS,
				FWD_FORMAT_ANSWER_PREPARE_PROGRESS,
				preparation->seqnum, preparation->folder,
				preparation->identification_string, chunk);
		if (ret < 0)
			ULOGE("firmwared_notify: %s", strerror(-ret));
	}
}

/* mounts the firmware so that the client can retrieve informations if needed */
static int mount_firmware(struct firmware *firmware)
{
	int ret;
	int ret2;
	struct io_process process;
	const char *mount_dir =
			folder_entity_get_base_workspace(&firmware->entity);

	ret = mkdir(mount_dir, 0755);
	if (ret == -1 && errno != EEXIST)
		ULOGW("mkdir: %m");
	if (ut_file_is_dir(firmware->path)) {
		ret = io_process_init_prepare_launch_and_wait(&process,
				&process_default_parameters, NULL, "/bin/mount",
				"--bind", firmware->path, mount_dir,
				NULL);
		/*
		 * The remount is necessary to make this bind mount read-only.
		 * Trying to pass the ro option directly to the previous command
		 * won't work, according to the man page, the mount options of
		 * the bind mount will be the same as those of the original
		 * mount.
		 */
		ret2 = io_process_init_prepare_launch_and_wait(&process,
				&process_default_parameters, NULL, "/bin/mount",
				"-o", "ro,remount,bind", firmware->path,
				mount_dir, NULL);
		if (ret2 < 0)
			ULOGW("remounting %s read-only failed: %s", mount_dir,
					strerror(-ret2));
	} else {
		ret = io_process_init_prepare_launch_and_wait(&process,
				&process_default_parameters, NULL, "/bin/mount",
				"-o", "ro,loop", firmware->path, mount_dir,
				NULL);
	}

	return ret;
}

static struct firmware *firmware_new(const char *path)
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

	firmware->entity.folder = folder_find(FIRMWARES_FOLDER_NAME);
	if (ut_file_is_dir(path)) {
		firmware->path = strdup(path);
		if (path == NULL) {
			ret = -errno;
			ULOGE("strdup: %m");
			goto err;
		}
	} else {
		ret = asprintf(&firmware->path, "%s/%s",
				firmware_repository_path, path);
		if (ret == -1) {
			firmware->path = NULL;
			ULOGE("asprintf error");
			errno = -ENOMEM;
			goto err;
		}
	}

	/* force sha1 computation while in parallel section */
	sha1 = compute_sha1(firmware);
	if (sha1 == NULL)
		goto err;

	ret = mount_firmware(firmware);
	if (ret < 0)
		ULOGW("read_firmware_info failed: %s\n", strerror(-ret));

	ULOGD("indexing firmware %s done", path);

	return firmware;
err:
	firmware_delete(&firmware);

	return NULL;
}

static void firmware_preparation_termination(struct io_process *process,
		pid_t pid, int status)
{
	int ret;
	struct firmware_preparation *firmware_preparation;
	struct preparation *preparation;
	struct firmware *firmware;
	char __attribute__((cleanup(ut_string_free)))*file_path = NULL;

	firmware_preparation = ut_container_of(process,
			struct firmware_preparation, process);
	io_mon_remove_source(firmwared_get_mon(),
		io_process_get_src(&firmware_preparation->process));

	preparation = &firmware_preparation->preparation;

	if (status != 0) {
		if (WIFSIGNALED(status)) {
			ULOGD("curl hook terminated on signal %s",
					strsignal(WTERMSIG(status)));
		} else {
			if (WTERMSIG(status) != SIGUSR1) {
				ULOGE("curl hook error, use absolute paths");
				ret = -EINVAL;
				goto err;
			}
		}
	}

	/* TODO the following may block a long time */
	firmware = firmware_new(firmware_preparation->destination_file);

	preparation->completion(preparation, &firmware->entity);

	return;
err:
	firmwared_notify(FWD_ANSWER_ERROR, FWD_FORMAT_ANSWER_ERROR,
			preparation->seqnum, -ret, strerror(-ret));

	preparation->completion(preparation, NULL);
}

static int firmware_preparation_start(struct preparation *preparation)
{
	int ret;
	struct firmware_preparation *firmware_preparation;
	struct firmware *firmware;
	char buf[0x200] = {0};
	char *pbuf = &buf[0];
	const char *uuid = buf + 5;
	const char *id = preparation->identification_string;

	if (ut_file_is_dir(id)) {
		firmware = get_from_path(id);
		if (firmware == NULL)
			firmware = firmware_new(id);
		return preparation->completion(preparation, &firmware->entity);
	}

	firmware_preparation = ut_container_of(preparation,
			struct firmware_preparation, preparation);

	ret = ut_process_read_from_output(&pbuf, 0x200, "\"%s\" \"%s\" "
			"\"%s\" \"%s\" \"%s\" \"%s\"",
			config_get(CONFIG_CURL_HOOK),
			"uuid",
			id,
			config_get(CONFIG_REPOSITORY_PATH),
			"", /* we don't know uuid yet, of course */
			config_get(CONFIG_VERBOSE_HOOK_SCRIPTS));
	if (ret < 0) {
		ULOGE("uuid retrieval failed");
		return ret;
	}

	ut_string_rstrip(pbuf);
	/*
	 * the uuid corresponds to an already registered firmware, nothing to
	 * do and we consider it a success
	 */
	if (uuid_already_registered(uuid)) {
		firmware = get_from_uuid(uuid);
		return preparation->completion(preparation, &firmware->entity);
	}

	ret = io_process_init_prepare_and_launch(&firmware_preparation->process,
			&(struct io_process_parameters){
				.stdout_sep_cb = preparation_progress_sep_cb,
				.out_sep1 = '\n',
				.out_sep2 = IO_SRC_SEP_NO_SEP2,
				.stderr_sep_cb = log_warn_src_sep_cb,
				.err_sep1 = '\n',
				.err_sep2 = IO_SRC_SEP_NO_SEP2,
				.timeout = PREPARATION_TIMEOUT,
				.signum = PREPARATION_TIMEOUT_SIGNAL,
			},
			firmware_preparation_termination,
			config_get(CONFIG_CURL_HOOK),
			"fetch",
			id,
			config_get(CONFIG_REPOSITORY_PATH),
			uuid,
			config_get(CONFIG_VERBOSE_HOOK_SCRIPTS),
			NULL);
	if (ret < 0)
		return ret;

	return io_mon_add_source(firmwared_get_mon(),
			io_process_get_src(&firmware_preparation->process));
}

static void firmware_preparation_abort(struct preparation *preparation)
{
	struct firmware_preparation *firmware_preparation;
	struct io_process *process;

	firmware_preparation = ut_container_of(preparation,
			struct firmware_preparation, preparation);
	process = &firmware_preparation->process;

	ULOGE("[%s] abort preparation of '%s'", preparation->folder,
			preparation->identification_string);

	io_process_signal(process, SIGUSR1);
}

static struct preparation *firmware_get_preparation(void)
{
	struct firmware_preparation *firmware_preparation;

	firmware_preparation = calloc(1, sizeof(*firmware_preparation));
	if (firmware_preparation == NULL)
		return NULL;

	firmware_preparation->preparation.start = firmware_preparation_start;
	firmware_preparation->preparation.abort = firmware_preparation_abort;
	firmware_preparation->preparation.folder = FIRMWARES_FOLDER_NAME;

	return &firmware_preparation->preparation;
}

static void firmware_destroy_preparation(struct preparation **preparation)
{
	struct firmware_preparation *firmware_preparation;

	firmware_preparation = ut_container_of(*preparation,
			struct firmware_preparation, preparation);

	ut_string_free(&firmware_preparation->destination_file);
	memset(firmware_preparation, 0, sizeof(*firmware_preparation));
	free(firmware_preparation);
	*preparation = NULL;
}

struct folder_entity_ops firmware_ops = {
		.sha1 = firmware_sha1,
		.can_drop = firmware_can_drop,
		.drop = firmware_drop,
		.get_preparation = firmware_get_preparation,
		.destroy_preparation = firmware_destroy_preparation,
};

static int pattern_filter(const struct dirent *d)
{
	return fnmatch(FIRMWARE_MATCHING_PATTERN, d->d_name, 0) == 0;
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
	struct firmware __attribute__((cleanup(free_firmwares))) **firmwares =
			NULL;

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
			firmwares[i] = firmware_new(namelist[i]->d_name);
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

int firmwares_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = ut_file_mkdir("%s/" FIRMWARES_FOLDER_NAME, 0755,
			config_get(CONFIG_MOUNT_PATH));
	if (ret < 0 && ret != -EEXIST)
		ULOGW("couldn't mkdir the firmwares mount path");

	firmware_folder.name = FIRMWARES_FOLDER_NAME;
	memcpy(&firmware_folder.ops, &firmware_ops, sizeof(firmware_ops));
	ret = folder_register(&firmware_folder);
	if (ret < 0) {
		ULOGE("folder_register: %s", strerror(-ret));
		return ret;
	}
	folder_register_properties(FIRMWARES_FOLDER_NAME, firmware_properties);

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

const char *firmware_get_path(const struct firmware *firmware)
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

const char *firmware_get_uuid(struct firmware *firmware)
{
	const char *path;

	errno = EINVAL;
	if (firmware == NULL)
		return NULL;
	path = firmware_get_path(firmware);
	if (firmware->uuid == NULL) {
		if (ut_file_is_dir(path))
			firmware->uuid = strdup("");
		else
			firmware->uuid = fwd_read_uuid(path);
	}

	return firmware->uuid;
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
