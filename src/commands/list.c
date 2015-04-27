/**
 * @file list.c
 * @brief
 *
 * @date Apr 21, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <libpomp.h>

#include <ut_string.h>

#define ULOG_TAG firmwared_command_list
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_list);

#include "commands.h"
#include "folders.h"

#define COMMAND_NAME "LIST"

static int build_list(char **list, const struct folder *folder, unsigned *count)
{
	int ret;
	struct folder_entity *e = NULL;
	char *tmp = NULL;

	if (list == NULL || folder == NULL || count == NULL)
		return -EINVAL;

	*list = strdup("");
	if (*list == NULL) {
		ret = -errno;
		ULOGE("strdup: %s", strerror(-ret));
		return ret;
	}

	*count = 0;
	while ((e = folder_next(folder, e)) != NULL) {
		ret = asprintf(&tmp, "(%s, %s), %s", e->name,
				folder_entity_get_sha1(e), *list);
		if (ret < 0) {
			ULOGC("asprintf error");
			return -ENOMEM;
		}
		free(*list);
		*list = tmp;
		(*count)++;
	}
	if ((*list)[0] != '\0')
		(*list)[strlen((*list)) - 2] = '\0';

	return 0;
}

static int list_command_handler(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	struct folder *folder;
	char __attribute__((cleanup(ut_string_free))) *cmd = NULL;
	char __attribute__((cleanup(ut_string_free))) *folder_name = NULL;
	unsigned count;
	char __attribute__((cleanup(ut_string_free))) *list = NULL;

	ret = pomp_msg_read(msg, "%ms%ms", &cmd, &folder_name);
	if (ret < 0) {
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}
	folder = folder_find(folder_name);
	if (folder == NULL) {
		ret = -errno;
		ULOGE("folder_find: %s", strerror(-ret));
		return ret;
	}

	ret = build_list(&list, folder, &count);
	if (ret < 0) {
		ULOGE("build_list: %s", strerror(-ret));
		return ret;
	}

	return firmwared_answer(conn, msg, "%s%s%u%s", "LIST", folder_name,
			count, list);
}

static const struct command list_command = {
		.name = COMMAND_NAME,
		.help = "List the content of a given folder.",
		.synopsis = "FOLDER",
		.handler = list_command_handler,
};

static __attribute__((constructor)) void list_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&list_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void list_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
