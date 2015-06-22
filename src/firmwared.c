/**
 * @file firmwared.c
 * @brief
 *
 * @date Apr 17, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#define ULOG_TAG firmwared
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared);

#include <libpomp.h>

#include <ut_string.h>
#include <ut_utils.h>

#include "commands.h"
#include "folders.h"
#include "instances.h"
#include "firmwares.h"
#include "config.h"

struct firmwared {
	struct io_mon mon;
	struct io_src pomp_src;
	struct pomp_ctx *pomp;
	struct io_src_sig sig_src;
	bool loop;
	bool initialized;
};

static struct firmwared ctx;

static size_t setup_address(struct sockaddr_storage *addr_storage)
{
	struct sockaddr *addr = (struct sockaddr *)addr_storage;
	struct sockaddr_un *addr_un = (struct sockaddr_un *)addr;
	memset(&addr_storage, 0, sizeof(addr_storage));
	addr_un->sun_family = AF_UNIX;
	strncpy(addr_un->sun_path, config_get(CONFIG_SOCKET_PATH),
			sizeof(addr_un->sun_path));

	return sizeof(*addr_un);
}

static void event_cb(struct pomp_ctx *pomp, enum pomp_event event,
		struct pomp_conn *conn, const struct pomp_msg *msg,
		void *userdata)
{
	int ret;

	ULOGD("%s : event=%d(%s) conn=%p msg=%p", __func__,
			event, pomp_event_str(event), conn, msg);

	switch (event) {
	case POMP_EVENT_CONNECTED:
		break;

	case POMP_EVENT_DISCONNECTED:
		break;

	case POMP_EVENT_MSG:
		ret = command_invoke(conn, msg);
		if (ret < 0) {
			ULOGE("command_invoke: %s", strerror(-ret));
			return;
		}
		break;

	default:
		ULOGW("Unknown event : %d", event);
		break;
	}
}

static void change_sock_group_mode()
{
	int ret;
	struct group *g;
	const char *socket_path = config_get(CONFIG_SOCKET_PATH);

	g = getgrnam(FIRMWARED_GROUP);
	if (g == NULL) {
		ULOGE("socket %s group couldn't be changed to "FIRMWARED_GROUP,
				socket_path);
		return;
	}

	ret = chown(socket_path, -1, g->gr_gid);
	if (ret == -1) {
		ULOGE("chown(%s, -1, %jd) error", socket_path,
				(intmax_t)g->gr_gid);
		return;
	}
	ret = chmod(socket_path, 0660);
	if (ret == -1) {
		ULOGE("chmod(%s, 0660) error", socket_path);
		return;
	}
}

static void pomp_src_cb(struct io_src *src)
{
	struct firmwared *f = ut_container_of(src, typeof(*f), pomp_src);
	struct pomp_ctx *pomp = f->pomp;
	int ret;

	ret = pomp_ctx_process_fd(pomp);
	if (ret < 0) {
		ULOGE("pomp_ctx_wait_and_process : err=%d(%s)", ret,
				strerror(-ret));
		return;
	}
}

static void sig_src_cb(struct io_src_sig *sig, struct signalfd_siginfo *si)
{
	struct firmwared *f = ut_container_of(sig, typeof(*f), sig_src);

	ULOGI("Caught signal %d, exiting", si->ssi_signo);

	firmwared_stop();
}

int firmwared_init(void)
{
	int ret;
	struct sockaddr_storage addr_storage;
	size_t s;

	if (ctx.initialized != 0)
		return 0;

	ctx.loop = true;
	ctx.pomp = pomp_ctx_new(&event_cb, NULL);
	if (ctx.pomp == NULL) {
		ret = -errno;
		ULOGE("pomp_ctx_new failed");
		return ret;
	}

	s = setup_address(&addr_storage);
	ret = pomp_ctx_listen(ctx.pomp, (struct sockaddr *)&addr_storage, s);
	if (ret < 0) {
		ULOGE("pomp_ctx_listen : err=%d(%s)", ret, strerror(-ret));
		goto err;
	}
	change_sock_group_mode();

	ret = io_mon_init(&ctx.mon);
	if (ret < 0) {
		ULOGE("uv_loop_init: %s", strerror(-ret));
		goto err;
	}

	ret = io_src_init(&ctx.pomp_src, pomp_ctx_get_fd(ctx.pomp), IO_IN,
			pomp_src_cb);
	if (ret < 0) {
		ULOGE("io_src_init: %s", strerror(-ret));
		goto err;
	}
	ret = io_src_sig_init(&ctx.sig_src, sig_src_cb, SIGINT, SIGTERM,
			SIGQUIT, NULL /* guard */);
	if (ret < 0) {
		ULOGE("io_src_sig_init: %s", strerror(-ret));
		goto err;
	}

	ret = io_mon_add_sources(&ctx.mon, &ctx.pomp_src,
			io_src_sig_get_source(&ctx.sig_src), NULL /* guard */);
	if (ret < 0) {
		ULOGE("io_mon_add_sources: %s", strerror(-ret));
		goto err;
	}
	ctx.initialized = true;

	return 0;
err:
	firmwared_clean();

	return ret;
}

void firmwared_run(void)
{
	int ret;

	while (ctx.loop) {
		ret = io_mon_poll(&ctx.mon, -1);
		if (ret < 0) {
			ULOGE("io_mon_poll: %s", strerror(-ret));
			return;
		}
		folders_reap_preparations();
	}
}

void firmwared_stop(void)
{
	ctx.loop = false;
}

int firmwared_notify(uint32_t msgid, const char *fmt, ...)
{
	int ret;
	va_list args;

	if (ut_string_is_invalid(fmt))
		return -EINVAL;

	va_start(args, fmt);
	ret = pomp_ctx_sendv(ctx.pomp, msgid, fmt, args);
	va_end(args);

	return ret;
}

struct io_mon *firmwared_get_mon(void)
{
	return &ctx.mon;
}

void firmwared_clean(void)
{
	io_mon_remove_sources(&ctx.mon, io_src_sig_get_source(&ctx.sig_src),
			&ctx.pomp_src, NULL /* guard */);
	io_src_sig_clean(&ctx.sig_src);
	io_src_clean(&ctx.pomp_src);
	if (ctx.pomp != NULL) {
		pomp_ctx_stop(ctx.pomp);
		pomp_ctx_destroy(ctx.pomp);
		ctx.pomp = NULL;
	}
	memset(&ctx, 0, sizeof(ctx));

	unlink(config_get(CONFIG_SOCKET_PATH));
}
