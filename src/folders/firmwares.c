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

static void firmware_delete(struct firmware **firmware)
{
	struct firmware *f;

	if (firmware == NULL || *firmware == NULL)
		return;
	f = *firmware;

	ut_string_free(&f->path);
	ut_string_free(&f->hardware);
	ut_string_free(&f->product);
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

	ret = firmwared_notify((uint32_t)-1, "%s%s%s%s",
			"PREPARATION_PROGRESS", preparation->folder,
			preparation->identification_string, chunk);
	if (ret < 0)
		ULOGE("firmwared_notify: %s", strerror(-ret));
}

static int get_build_prop(struct firmware *firmware, const char *argz,
		size_t argz_len, char **out, const char *property)
{
	if (out == NULL)
		return -EINVAL;

	*out = envz_get(argz, argz_len, property);
	if (*out == NULL) {
		ULOGI("property \"%s\" not found", property);
		return 0;
	}
	*out = strdup(*out);
	if (*out == NULL)
		return -errno;
	ULOGD("%s read from build.prop : %s", property, *out);

	return 0;
}

static int extract_firmware_info(struct firmware *firmware, char *props)
{
	int ret;
	char __attribute__((cleanup(ut_string_free)))*argz = NULL;
	size_t argz_len = 0;

	ret = argz_create_sep(props, '\n', &argz, &argz_len);
	if (ret != 0) {
		argz = NULL;
		return ret;
	}

	ret = get_build_prop(firmware, argz, argz_len, &firmware->product,
			"ro.build.alchemy.product");
	if (ret < 0)
		return ret;

	return get_build_prop(firmware, argz, argz_len, &firmware->hardware,
			"ro.hardware");
}

/* mounts the firmware to read some informations it contains */
static int read_firmware_info(struct firmware *firmware)
{
	int ret;
	struct io_process process;
	int status;
	char mount_dir[] = "/tmp/firmwared_firmwareXXXXXX";
	char __attribute__((cleanup(ut_string_free)))*props = NULL;
	void termination_cb(struct io_src_pid *pid_src, pid_t pid, int s)
	{
		status = s;
	};

	if (mkdtemp(mount_dir) == NULL)
		return -errno;

	if (ut_file_is_dir(firmware->path)) {
		ret = io_process_init_prepare_launch_and_wait(&process,
				&process_default_parameters, termination_cb,
				"/bin/mount", "-o", "ro", "--bind",
				firmware->path, mount_dir, NULL);
	} else  {
		ret = io_process_init_prepare_launch_and_wait(&process,
				&process_default_parameters, termination_cb,
				"/bin/mount", "-o", "ro,loop", firmware->path,
				mount_dir, NULL);
	}
	if (ret < 0) {
		ULOGE("io_process_init_prepare_launch_and_wait: %s",
				strerror(-ret));
		goto out;
	}

	ret = ut_file_to_string("%s/etc/build.prop", &props, mount_dir);
	if (ret < 0) {
		props = NULL;
		ULOGE("ut_file_to_vstring: %s", strerror(-ret));
		goto out;
	}

	ret = extract_firmware_info(firmware, props);
out:
	io_process_init_prepare_launch_and_wait(&process,
			&process_default_parameters, termination_cb,
			"/bin/umount", mount_dir, NULL);
	rmdir(mount_dir);

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
			ULOGE("asprintf error");
			errno = -ENOMEM;
			goto err;
		}
	}

	/* force sha1 computation while in parallel section */
	sha1 = compute_sha1(firmware);
	if (sha1 == NULL)
		goto err;

	ret = read_firmware_info(firmware);
	if (ret < 0)
		ULOGW("read_firmware_info failed: %s\n", strerror(-ret));

	ULOGD("indexing firmware %s done", path);

	return firmware;
err:
	firmware_delete(&firmware);

	return NULL;
}

static void firmware_preparation_termination(struct io_src_pid *pid_src,
		pid_t pid, int status)
{
	int ret;
	struct firmware_preparation *firmware_preparation;
	struct preparation *preparation;
	struct io_process *process;
	struct firmware *firmware;
	char __attribute__((cleanup(ut_string_free)))*file_path = NULL;

	process = ut_container_of(pid_src, struct io_process, pid_src);
	firmware_preparation = ut_container_of(process,
			struct firmware_preparation, process);
	io_mon_remove_source(firmwared_get_mon(),
		io_process_get_src(&firmware_preparation->process));

	preparation = &firmware_preparation->preparation;

	if (status != 0) {
		ULOGE("curl hook execution status is non-zero, "
				"please keep in mind that relative paths "
				"aren't accepted");
		ret = -EINVAL;
		goto err;
	}

	ret = asprintf(&file_path, "%s"FIRMWARE_SUFFIX,
			basename(preparation->identification_string));
	if (ret < 0) {
		file_path = NULL;
		ULOGC("asprintf failure");
		ret = -ENOMEM;
		goto err;
	}
	// TODO the following may block a long time
	firmware = firmware_new(file_path);

	preparation->completion(preparation, &firmware->entity);

	return;
err:
	firmwared_notify(preparation->msgid, "%s%d%s", "ERROR", -ret,
			strerror(-ret));

	preparation->completion(preparation, NULL);
}

static int firmware_preparation_start(struct preparation *preparation)
{
	int ret;
	struct firmware_preparation *firmware_preparation;
	struct firmware *firmware;

	if (ut_file_is_dir(preparation->identification_string)) {
		firmware = firmware_new(preparation->identification_string);
		return preparation->completion(preparation, &firmware->entity);
	}

	firmware_preparation = ut_container_of(preparation,
			struct firmware_preparation, preparation);

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
			preparation->identification_string,
			config_get(CONFIG_REPOSITORY_PATH),
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

	io_process_kill(process);
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
