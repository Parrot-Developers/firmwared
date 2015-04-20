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

#include "commands.h"

#define SOCK_GROUP "firmwared"
#define DEFAULT_SOCKET_PATH "/var/run/firmwared.sock"
static char *socket_path = DEFAULT_SOCKET_PATH;

static size_t setup_address(struct sockaddr_storage *addr_storage)
{
	struct sockaddr *addr = (struct sockaddr *)addr_storage;
	struct sockaddr_un *addr_un = (struct sockaddr_un *)addr;
	memset(&addr_storage, 0, sizeof(addr_storage));
	addr_un->sun_family = AF_UNIX;
	strncpy(addr_un->sun_path, socket_path, sizeof(addr_un->sun_path));

	return sizeof(*addr_un);
}

static void event_cb(struct pomp_ctx *pomp, enum pomp_event event,
		struct pomp_conn *conn, const struct pomp_msg *msg,
		void *userdata)
{
	struct firmwared *ctx = userdata;
	int ret;

	ULOGD("%s : event=%d(%s) conn=%p msg=%p", __func__,
			event, pomp_event_str(event), conn, msg);

	if (ctx == NULL) {
		ULOGC("%s : invalid userdata parameter", __func__);
		return;
	}

	switch (event) {
	case POMP_EVENT_CONNECTED:
		break;

	case POMP_EVENT_DISCONNECTED:
		break;

	case POMP_EVENT_MSG:
		ret = command_invoke(ctx, msg);
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

	g = getgrnam(SOCK_GROUP);
	if (g == NULL) {
		ULOGE("socket %s group couldn't be changed to "SOCK_GROUP,
				socket_path);
		return;
	}

	ret = chown(DEFAULT_SOCKET_PATH, -1, g->gr_gid);
	if (ret == -1) {
		ULOGE("chown(%s, -1, %jd) error", socket_path,
				(intmax_t)g->gr_gid);
		return;
	}
	ret = chmod(DEFAULT_SOCKET_PATH, 0660);
	if (ret == -1) {
		ULOGE("chmod(%s, 0660) error", socket_path);
		return;
	}
}

int firmwared_init(struct firmwared *ctx)
{
	int ret;
	struct sockaddr_storage addr_storage;
	size_t s;

	ctx->pomp = pomp_ctx_new(&event_cb, ctx);
	if (ctx->pomp == NULL) {
		ULOGE("pomp_ctx_new failed");
		return -ENOMEM;
	}

	s = setup_address(&addr_storage);
	ret = pomp_ctx_listen(ctx->pomp, (struct sockaddr *)&addr_storage, s);
	if (ret < 0) {
		ULOGE("pomp_ctx_listen : err=%d(%s)", ret, strerror(-ret));
		goto err;
	}
	change_sock_group_mode();

	ctx->decoder = pomp_decoder_new();
	if (ctx->decoder == NULL) {
		ULOGE("pomp_decoder_new failed");
		goto err;
	}

	ctx->loop = true;

	return 0;
err:
	firmwared_clean(ctx);

	return ret;
}

void firmwared_run(struct firmwared *ctx)
{
	int ret;

	while (ctx->loop) {
		ret = pomp_ctx_wait_and_process(ctx->pomp, -1);
		if (ret < 0) {
			ULOGE("pomp_ctx_wait_and_process : err=%d(%s)", ret,
					strerror(-ret));
			return;
		}
	}
}

void firmwared_clean(struct firmwared *ctx)
{
	if (ctx->pomp != NULL) {
		pomp_ctx_stop(ctx->pomp);
		pomp_ctx_destroy(ctx->pomp);
		ctx->pomp = NULL;
		if (ctx->decoder != NULL) {
			pomp_decoder_destroy(ctx->decoder);
			ctx->decoder = NULL;
		}
	}
	memset(ctx, 0, sizeof(*ctx));

	unlink(socket_path);

}

uint32_t firmwared_get_msg_id(struct firmwared *ctx)
{
	if (ctx == NULL)
		return UINT32_MAX;

	if (ctx->msg_id == UINT32_MAX)
		ctx->msg_id = 0;

	return ctx->msg_id++;
}
