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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>

#include <grp.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <signal.h>

#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <openssl/sha.h>

#include <uv.h>

#define ULOG_TAG firmwared_instances
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_instances);

#include <pidwatch.h>

#include <ut_utils.h>
#include <ut_string.h>
#include <ut_file.h>
#include <ut_process.h>
#include <ut_bits.h>

#include <ptspair.h>

#include "folders.h"
#include "instances.h"
#include "utils.h"
#include "config.h"
#include "firmwared.h"
#include "apparmor.h"

/*
 * this hardcoded value could be a modifiable parameter, but it really adds to
 * much complexity to the code and so isn't worth the effort
 */
#define NET_BITS 24

static ut_bit_field_t indices;

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
};

#define to_instance(p) ut_container_of(p, struct instance, entity)

struct pid_cb_data {
	struct firmwared *firmwared;
	struct instance *instance;
};

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
	return instance_get_sha1(to_instance(entity));
}

static void clean_paths(struct instance *instance)
{
	ut_string_free(&instance->base_workspace);
	ut_string_free(&instance->ro_mount_point);
	ut_string_free(&instance->rw_dir);
	ut_string_free(&instance->union_mount_point);
}

static int invoke_mount_helper(struct instance *instance, const char *action,
		bool only_unregister)
{
	return ut_process_vsystem("\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" "
				"\"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
			config_get(CONFIG_MOUNT_HOOK),
			action,
			instance->base_workspace,
			instance->ro_mount_point,
			instance->rw_dir,
			instance->union_mount_point,
			instance->firmware_path,
			instance->firmware_sha1,
			only_unregister ? "true" : "false",
			config_get(CONFIG_PREVENT_REMOVAL),
			config_get(CONFIG_USE_AUFS));
}

static int invoke_net_helper(struct instance *instance, const char *action)
{
	return ut_process_vsystem("\"%s\" \"%s\" \"%s\" \"%s\" \"%"PRIu8"\" "
			"\"%s\" \"%d\" \"%jd\"",
			config_get(CONFIG_NET_HOOK),
			action,
			config_get(CONFIG_CONTAINER_INTERFACE),
			config_get(CONFIG_HOST_INTERFACE_PREFIX),
			instance->id,
			config_get(CONFIG_NET_FIRST_TWO_BYTES),
			NET_BITS,
			(intmax_t)instance->pid);
}

static void clean_mount_points(struct instance *instance, bool only_unregister)
{
	int ret;

	ret = invoke_mount_helper(instance, "clean", only_unregister);
	if (ret != 0)
		ULOGE("invoke_mount_helper clean returned %d", ret);
	clean_paths(instance);
}

static void clean_instance(struct instance *i, bool only_unregister)
{
	struct pid_cb_data *data;

	if (!config_get_bool(CONFIG_DISABLE_APPARMOR))
		apparmor_remove_profile(instance_get_name(i));
	ut_process_sync_clean(&i->sync);
	data = i->pidfd_handle.data;

	uv_poll_stop(&i->pidfd_handle);
	uv_close((uv_handle_t *)&i->pidfd_handle, NULL);
	/* uv_close is asynchronous and forces us to run the loop once */
	uv_run(&data->firmwared->loop, UV_RUN_NOWAIT);
	free(i->pidfd_handle.data);
	ut_file_fd_close(&i->pidfd);
	ptspair_clean(&i->ptspair);
	clean_mount_points(i, only_unregister);

	ut_string_free(&i->firmware_sha1);
	ut_string_free(&i->firmware_path);
	ut_bit_field_release_index(&indices, i->id);
	memset(i, 0, sizeof(*i));
}

static void instance_delete(struct instance **instance, bool only_unregister)
{
	struct instance *i;

	if (instance == NULL || *instance == NULL)
		return;
	i = *instance;

	clean_instance(i, only_unregister);

	free(i);
	*instance = NULL;
}

static bool instance_is_running(struct instance *instance)
{
	return instance == NULL ? false : instance->state != INSTANCE_READY;
}

static bool instance_can_drop(struct folder_entity *entity)
{
	struct instance *instance = to_instance(entity);

	return !instance_is_running(instance);
}

static int instance_drop(struct folder_entity *entity, bool only_unregister)
{
	struct instance *instance = to_instance(entity);

	ULOGD("%s", __func__);

	if (instance_is_running(instance)) {
		ULOGW("instance %s still running, try to kill an wait for it",
				instance_get_name(instance));
		instance_kill(instance, (uint32_t)-1);
		sleep(1);
	}
	instance_delete(&instance, only_unregister);

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

static char *instance_get_info(const struct folder_entity *entity)
{
	int ret;
	struct instance *instance = to_instance(entity);
	char *info;

	ret = asprintf(&info, "id: %"PRIu8"\n"
			"pid: %jd\n"
			"state: %s\n"
			"firmware_path: %s\n"
			"base_workspace: %s\n"
			"pts: %s\n"
			"firmware_sha1: %s\n"
			"time: %s",
			instance->id,
			(intmax_t)instance->pid,
			instance_state_to_str(instance->state),
			instance->firmware_path,
			instance->base_workspace,
			ptspair_get_path(&instance->ptspair, PTSPAIR_FOO),
			instance->firmware_sha1,
			ctime(&instance->time));
	if (ret < 0) {
		ULOGE("asprintf error");
		errno = ENOMEM;
		return NULL;
	}

	return info;
}

struct folder_entity_ops instance_ops = {
		.sha1 = instance_sha1,
		.can_drop = instance_can_drop,
		.drop = instance_drop,
		.get_info = instance_get_info,
};

static int init_paths(struct instance *instance)
{
	int ret;

	ret = asprintf(&instance->base_workspace, "%s/%s",
			config_get(CONFIG_MOUNT_PATH),
			instance_get_sha1(instance));
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
	ret = invoke_mount_helper(instance, "init", false);
	if (ret != 0) {
		ULOGE("invoke_mount_helper init returned %d", ret);
		ret = -ENOTRECOVERABLE;
		goto err;
	}

	return 0;
err:
	clean_mount_points(instance, false);

	return ret;
}

static void pidfd_uv_poll_cb(uv_poll_t *handle, int status, int events)
{
	int ret;
	int program_status;
	struct pid_cb_data *data = handle->data;
	struct instance *instance = data->instance;
	struct firmwared *firmwared = data->firmwared;

	ret = pidwatch_wait(instance->pidfd, &program_status);
	if (ret < 0) {
		ULOGE("pidwatch_wait : err=%d(%s)", ret, strerror(-ret));
		return;
	}

	pidwatch_set_pid(instance->pidfd, instance->pid);
	ULOGD("process %jd exited with status %d", (intmax_t)instance->pid,
			program_status);

	ret = waitpid(instance->pid, &program_status, WNOHANG);
	if (ret < 0) {
		ULOGE("waitpid : err=%d(%s)", ret, strerror(-ret));
		return;
	}
	ULOGD("waitpid said %d", program_status);

	instance->state = INSTANCE_READY;
	instance->pid = 0;

	ret = firmwared_notify(firmwared, instance->killer_msgid, "%s%s%s",
			"DEAD", instance_get_sha1(instance),
			instance_get_name(instance));
	instance->killer_msgid = (uint32_t) -1;
	if (ret < 0)
		ULOGE("firmwared_notify : err=%d(%s)", ret, strerror(-ret));
}

static void ptspair_uv_poll_cb(uv_poll_t *handle, int status, int events)
{
	int ret;
	struct ptspair *ptspair = handle->data;

	ret = ptspair_process_events(ptspair);
	if (ret < 0) {
		ULOGE("ptspair_process_events : err=%d(%s)", ret,
				strerror(-ret));
		return;
	}
}

static int setup_container(struct instance *instance)
{
	int ret;
	int flags;

	/*
	 * use our own namespace for IPC, networking, mount points and uts
	 * (hostname and domain name), the pid namespace will be set up only
	 * after we have no more fork() (read: system()) calls to do for the
	 * setup
	 */
	flags = CLONE_FILES | CLONE_NEWIPC | CLONE_NEWNET |
			CLONE_NEWNS | CLONE_NEWUTS | CLONE_SYSVSEM;
	ret = unshare(flags);
	if (ret < 0) {
		ret = -errno;
		ULOGE("unshare: %m");
		return ret;
	}

	/*
	 * although we have a new mount points namespace, it is still necessary
	 * to make them private, recursively, so that changes aren't propagated
	 * to the parent namespace
	 */
	ret = mount(NULL, "/", NULL, MS_REC|MS_PRIVATE, NULL);
	if (ret < 0) {
		ret = -errno;
		ULOGE("cannot make \"/\" private: %m");
		return ret;
	}

	return 0;
}

static int setup_chroot(struct instance *instance)
{
	int ret;

	ret = chroot(instance->union_mount_point);
	if (ret < 0) {
		ret = -errno;
		ULOGE("chroot(%s): %m", instance->union_mount_point);
		return ret;
	}
	ret = chdir("/");
	if (ret < 0) {
		ret = -errno;
		ULOGE("chdir(/): %m");
		return ret;
	}

	return 0;
}

static int build_args(struct instance *instance, char ***argv)
{
	int ret;
	const char *pts;
	int i;

	*argv= calloc(5, sizeof(**argv));
	if (*argv == NULL)
		return -errno;

	// TODO load this from the firmware's config file
	i = 0;
	(*argv)[i++] = strdup("/sbin/boxinit");
	if ((*argv)[0] == NULL)
		goto err;

	(*argv)[i++] = strdup("ro.hardware=mk3_sim_pc");
	if ((*argv)[0] == NULL)
		goto err;

	(*argv)[i++] = strdup("ro.debuggable=1");
	if ((*argv)[0] == NULL)
		goto err;

	pts = ptspair_get_path(&instance->ptspair, PTSPAIR_BAR);
	ret = asprintf(*argv + i++, "ro.boot.console=%s", pts + 4);
	if (ret < 0) {
		errno = ENOMEM;
		(*argv)[i] = 0;
		goto err;
	}

	return 0;
err:

	*argv = NULL;

	return -errno;
}

static void destroy_args(char ***argv)
{
	char **arg;

	if (argv == NULL || *argv == NULL)
		return;

	arg = *argv;
	while (*arg != NULL) {
		ut_string_free(arg);
		arg++;
	}

	free(*argv);
	*argv = NULL;
}

static void launch_pid_1(struct instance *instance, int fd, sigset_t *mask)
{
	int ret;
	int i;
	const char *hostname;
	char __attribute__((cleanup(destroy_args)))**argv = NULL;
	const char *sha1;

	sha1 = instance_get_sha1(instance);
	ret = ut_process_change_name("pid-1-%s", sha1);
	if (ret < 0)
		ULOGE("ut_process_change_name(pid-1-%s): %s", sha1,
				strerror(-ret));

	ULOGI("%s", __func__);

	ret = build_args(instance, &argv);
	if (ret < 0) {
		ULOGE("build_args: %s", strerror(-ret));
		_exit(EXIT_FAILURE);
	}
	/* we need to be able to differentiate instances by hostnames */
	hostname = instance_get_name(instance);
	ret = sethostname(hostname, strlen(hostname));
	if (ret < 0)
		ULOGE("sethostname(%s): %m", hostname);

	/* be sure we die if our parent does */
	ret = prctl(PR_SET_PDEATHSIG, SIGKILL);
	if (ret < 0)
		ULOGE("prctl(PR_SET_PDEATHSIG, SIGKILL): %m");

	if (fd >= 0) {
		ret = dup2(fd, STDIN_FILENO);
		if (ret < 0)
			ULOGE("dup2(fd, STDIN_FILENO): %m");
		ret = dup2(fd, STDOUT_FILENO);
		if (ret < 0)
			ULOGE("dup2(fd, STDOUT_FILENO): %m");
		ret = dup2(fd, STDERR_FILENO);
		if (ret < 0)
			ULOGE("dup2(fd, STDERR_FILENO): %m");
	}
	/* re-enable the signals previously blocked */
	ret = sigprocmask(SIG_UNBLOCK, mask, NULL);
	if (ret == -1) {
		ULOGE("sigprocmask: %m");
		_exit(EXIT_FAILURE);
	}
	for (i = sysconf(_SC_OPEN_MAX) - 1; i > 2; i--)
		close(i);
	/* from here, ulog doesn't work anymore */
	ret = execv(argv[0], argv);
	if (ret < 0)
		/*
		 * if one want to search for potential failure of the execve
		 * call, he has to read in the instances pts
		 */
		fprintf(stderr, "execv: %m");

	_exit(EXIT_FAILURE);
}

static void launch_instance(struct instance *instance)
{
	int ret;
	pid_t pid;
	int fd;
	const char *sha1;
	char buf[0x200];
	const char *instance_name;
	int sfd;
	sigset_t mask;
	struct signalfd_siginfo si;
	int status;

	instance_name = instance_get_name(instance);
	sha1 = instance_get_sha1(instance);
	ret = ut_process_change_name("monitor-%s", sha1);
	if (ret < 0)
		ULOGE("ut_process_change__name(monitor-%s): %s", sha1,
				strerror(-ret));

	ULOGI("%s \"%s\"", __func__, instance->firmware_path);

	fd = open(ptspair_get_path(&instance->ptspair, PTSPAIR_BAR), O_RDWR);
	if (fd < 0) {
		ULOGE("open: %m");
		_exit(EXIT_FAILURE);
	}
	ret = setup_container(instance);
	if (ret < 0) {
		ULOGE("setup_container: %m");
		_exit(EXIT_FAILURE);
	}
	ret = ut_process_sync_child_unlock(&instance->sync);
	if (ret < 0)
		ULOGE("ut_process_sync_child_unlock: parent/child "
				"synchronisation failed: %s", strerror(-ret));
	ret = ut_process_sync_child_lock(&instance->sync);
	if (ret < 0)
		ULOGE("ut_process_sync_child_lock: parent/child "
				"synchronisation failed: %s", strerror(-ret));
	ret = invoke_net_helper(instance, "config");
	if (ret != 0) {
		ULOGE("invoke_net_helper returned %d", ret);
		_exit(EXIT_FAILURE);
	}
	if (!config_get_bool(CONFIG_DISABLE_APPARMOR)) {
		ret = apparmor_change_profile(instance_name);
		if (ret < 0) {
			ULOGE("apparmor_change_profile: %s", strerror(-ret));
			_exit(EXIT_FAILURE);
		}
	}
	ret = setup_chroot(instance);
	if (ret < 0) {
		ULOGE("setup_chroot: %m");
		_exit(EXIT_FAILURE);
	}

	/*
	 * at last, setup the pid namespace, no more fork allowed apart from
	 * pid 1
	 */
	ret = unshare(CLONE_NEWPID);
	if (ret < 0) {
		ULOGE("unshare: %m");
		_exit(EXIT_FAILURE);
	}

	sigemptyset(&mask);
	ret = sigaddset(&mask, SIGUSR1);
	if (ret == -1) {
		ULOGE("sigaddset: %m");
		_exit(EXIT_FAILURE);
	}
	ret = sigaddset(&mask, SIGCHLD);
	if (ret == -1) {
		ULOGE("sigaddset: %m");
		_exit(EXIT_FAILURE);
	}
	ret = sigprocmask(SIG_BLOCK, &mask, NULL);
	if (ret == -1) {
		ULOGE("sigprocmask: %m");
		_exit(EXIT_FAILURE);
	}
	sfd = signalfd(-1, &mask, SFD_CLOEXEC);
	if (sfd == -1) {
		ULOGE("signalfd: %m");
		_exit(EXIT_FAILURE);
	}

	pid = fork();
	if (pid < 0) {
		ULOGE("fork: %m");
		_exit(EXIT_FAILURE);
	}
	if (pid == 0)
		launch_pid_1(instance, fd, &mask);
	close(fd);

	ret = TEMP_FAILURE_RETRY(read(sfd, &si, sizeof(si)));
	if (ret == -1)
		ULOGE("read: %m");
	close(sfd);

	ret = kill(pid, SIGKILL);
	if (ret == -1)
		ULOGE("kill: %m");

	ret = waitpid(pid, &status, 0);
	if (ret < 0) {
		_exit(EXIT_FAILURE);
		ULOGE("waitpid: %m");
	}
	if (WIFEXITED(status))
		ULOGC("program exited with status %d", WEXITSTATUS(status));

	/*
	 * check that hostname hasn't changed, it could break some things like
	 * ulog's pseudo-namespacing
	 */
	ret = gethostname(buf, strlen(instance_name) + 1);
	if (ret < 0)
		ULOGW("gethostname: %m");
	else
		if (!ut_string_match(buf, instance_name))
			ULOGW("hostname has been changed during the life of the"
					"instance, this is bad as it can break"
					"some functionalities like ulog's "
					"pseudo name-spacing");

	ULOGI("instance %s terminated with status %d", instance_name, status);

	_exit(EXIT_SUCCESS);
}

static int setup_pts(struct ptspair *ptspair, enum pts_index index)
{
	int ret;
	struct group *g;
	const char *pts_path = ptspair_get_path(ptspair, index);

	g = getgrnam(FIRMWARED_GROUP);
	if (g == NULL) {
		ret = -errno;
		ULOGE("getgrnam(%s): %s", pts_path, strerror(-ret));
		return ret;
	}
	ret = chown(pts_path, -1, g->gr_gid);
	if (ret == -1) {
		ret = -errno;
		ULOGE("chown(%s, -1, %jd): %s", pts_path, (intmax_t)g->gr_gid,
				strerror(-ret));
		return ret;
	}
	ret = chmod(pts_path, 0660);
	if (ret == -1) {
		ret = -errno;
		ULOGE("chmod(%s, 0660): %s", pts_path, strerror(-ret));
		return ret;
	}

	return 0;
}

static int setup_ptspair(struct ptspair *ptspair)
{
	int ret;

	ret = ptspair_init(ptspair);
	if (ret < 0)
		return ret;
	ret = setup_pts(ptspair, PTSPAIR_FOO);
	if (ret < 0) {
		ULOGE("init_pts foo: %s", strerror(-ret));
		return ret;
	}

	return setup_pts(ptspair, PTSPAIR_BAR);
}

static int init_instance(struct instance *instance, struct firmwared *firmwared,
		const char *path, const char *sha1)
{
	int ret;
	struct pid_cb_data *data;

	instance->id = ut_bit_field_claim_free_index(&indices);
	if (instance->id == UT_BIT_FIELD_INVALID_INDEX) {
		ULOGE("ut_bit_field_claim_free_index: No free index");
		return -ENOMEM;
	}
	instance->time = time(NULL);
	instance->pid = 0;
	instance->state = INSTANCE_READY;
	instance->killer_msgid = (uint32_t) -1;
	instance->firmware_path = strdup(path);
	instance->firmware_sha1 = strdup(sha1);
	if (instance->firmware_path == NULL ||
			instance->firmware_sha1 == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	ret = init_mount_points(instance);
	if (ret < 0) {
		ULOGE("install_mount_points");
		errno = -ret;
		goto err;
	}

	ret = setup_ptspair(&instance->ptspair);
	if (ret < 0)
		ULOGE("init_ptspair: %s", strerror(-ret));
	instance->ptspair_handle.data = &instance->ptspair;
	ret = uv_poll_init(firmwared_get_uv_loop(firmwared),
			&instance->ptspair_handle,
			ptspair_get_fd(&instance->ptspair));
	if (ret < 0) {
		ULOGE("uv_poll_init: %s", strerror(-ret));
		goto err;
	}
	ret = uv_poll_start(&instance->ptspair_handle, UV_READABLE,
			ptspair_uv_poll_cb);
	if (ret < 0) {
		ULOGE("uv_poll_start: %s", strerror(-ret));
		goto err;
	}

	ret = folder_store(INSTANCES_FOLDER_NAME, &instance->entity);
	if (ret < 0) {
		ULOGE("folder_store: %s", strerror(-ret));
		goto err;
	}
	instance->pidfd = pidwatch_create(SOCK_CLOEXEC | SOCK_NONBLOCK);
	if (instance->pidfd == -1) {
		ret = -errno;
		ULOGE("pidwatch_create: %m");
		goto err;
	}

	ret = uv_poll_init(firmwared_get_uv_loop(firmwared),
			&instance->pidfd_handle, instance->pidfd);
	if (ret < 0) {
		ULOGE("uv_poll_init: %s", strerror(-ret));
		goto err;
	}
	data = calloc(1, sizeof(*data));
	if (data == NULL) {
		ret = -errno;
		ULOGE("calloc: %m");
		goto err;
	}
	data->instance = instance;
	data->firmwared = firmwared;
	instance->pidfd_handle.data = data;
	ret = uv_poll_start(&instance->pidfd_handle, UV_READABLE,
			pidfd_uv_poll_cb);
	if (ret < 0) {
		ULOGE("uv_poll_start: %s", strerror(-ret));
		goto err;
	}

	ret = ut_process_sync_init(&instance->sync, true);
	if (ret < 0){
		ULOGE("ut_process_sync_init: %s", strerror(-ret));
		goto err;
	}
	if (!config_get_bool(CONFIG_DISABLE_APPARMOR)) {
		ret = apparmor_load_profile(instance->base_workspace,
				instance_get_name(instance));
		if (ret < 0) {
			ULOGE("apparmor_load_profile: %s", strerror(-ret));
			goto err;
		}
	}

	return 0;
err:
	clean_instance(instance, false);

	return ret;

}

static int get_id(struct folder_entity *entity, char **value)
{
	int ret;
	struct instance *instance;

	if (entity == NULL || value == NULL)
		return -EINVAL;
	instance = to_instance(entity);

	ret = asprintf(value, "%"PRIu8, instance->id);
	if (ret < 0) {
		*value = NULL;
		ULOGE("asprintf error");
		return -ENOMEM;
	}

	return 0;
}

static struct folder_property id_property = {
		.name = "id",
		.get = get_id,
};

int instances_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	instances_folder.name = INSTANCES_FOLDER_NAME;
	memcpy(&instances_folder.ops, &instance_ops, sizeof(instance_ops));
	ret = folder_register(&instances_folder);
	if (ret < 0) {
		ULOGE("folder_register: %s", strerror(-ret));
		return ret;
	}
	folder_register_property(INSTANCES_FOLDER_NAME, &id_property);

	return 0;
}

struct instance *instance_new(struct firmwared *firmwared, const char *path,
		const char *sha1)
{
	int ret;
	struct instance *instance;

	instance = calloc(1, sizeof(*instance));
	if (instance == NULL)
		return NULL;

	ret = init_instance(instance, firmwared, path, sha1);
	if (ret < 0) {
		ULOGE("instance_init: %s", strerror(-ret));
		goto err;
	}

	return instance;
err:
	instance_delete(&instance, false);

	errno = -ret;

	return NULL;
}

struct instance *instance_from_entity(struct folder_entity *entity)
{
	errno = -EINVAL;
	if (entity == NULL)
		return NULL;

	return to_instance(entity);
}

int instance_start(struct instance *instance)
{
	int ret;

	if (instance == NULL)
		return -EINVAL;

	if (instance->state != INSTANCE_READY) {
		ULOGW("wrong state %s for instance %s",
				instance_state_to_str(instance->state),
				instance_get_sha1(instance));
		return -EBUSY;
	}

	/*
	 * the veth pair must be re-created at each instance startup, because it
	 * is automatically deleted at the namespace's destruction
	 */
	ret = invoke_net_helper(instance, "create");
	if (ret != 0) {
		ULOGE("invoke_net_helper returned %d", ret);
		return -EBUSY;
	}

	instance->state = INSTANCE_STARTED;
	ptspair_cooked(&instance->ptspair, PTSPAIR_BAR);
	instance->pid = fork();
	if (instance->pid == -1) {
		ret = -errno;
		ULOGE("fork: %m");
		return ret;
	}
	if (instance->pid == 0)
		launch_instance(instance); /* in child */
	/* in parent */
	ret = ut_process_sync_parent_lock(&instance->sync);
	if (ret < 0)
		ULOGE("ut_process_sync_parent_lock: parent/child "
				"synchronisation failed: %s", strerror(-ret));
	ret = invoke_net_helper(instance, "assign");
	if (ret != 0)
		ULOGE("invoke_net_helper returned %d", ret);
	ret = ut_process_sync_parent_unlock(&instance->sync);
	if (ret < 0)
		ULOGE("ut_process_sync_parent_unlock: parent/child "
				"synchronisation failed: %s", strerror(-ret));

	ret = pidwatch_set_pid(instance->pidfd, instance->pid);
	if (instance->pid == -1) {
		ULOGE("pidwatch_set_pid: %s", strerror(-ret));
		return ret;
	}

	return 0;
}

int instance_kill(struct instance *instance, uint32_t killer_msgid)
{
	int ret;

	if (instance == 0)
		return -EINVAL;

	if (instance->state != INSTANCE_STARTED)
		return -ECHILD;

	instance->state = INSTANCE_STOPPING;
	instance->killer_msgid = killer_msgid;
	ret = kill(instance->pid, SIGUSR1);
	if (ret < 0) {
		ret = -errno;
		ULOGE("kill: %m");
	}

	return ret;
}


const char *instance_get_sha1(struct instance *instance)
{
	errno = EINVAL;
	if (instance == NULL)
		return NULL;

	return compute_sha1(instance);
}

const char *instance_get_name(const struct instance *instance)
{
	errno = EINVAL;
	if (instance == NULL)
		return NULL;

	return instance->entity.name;
}

void instances_cleanup(void)
{
	ULOGD("%s", __func__);

	/*
	 * instances destruction is managed by instance_drop, called on each
	 * instance by folder_unregister
	 */
	folder_unregister(INSTANCES_FOLDER_NAME);
}
