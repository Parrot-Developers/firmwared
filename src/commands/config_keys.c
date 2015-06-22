/**
 * @file config_keys.c
 * @brief
 *
 * @date June 12, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_config_keys
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_config_keys);

#include <ut_string.h>

#include "commands.h"
#include "folders.h"

#define COMMAND_NAME "CONFIG_KEYS"

static int config_keys_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *list = NULL;
	char __attribute__((cleanup(ut_string_free))) *cmd = NULL;

	ret = pomp_msg_read(msg, "%ms", &cmd);
	if (ret < 0) {
		cmd = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	list = config_list_keys();
	if (list == NULL) {
		list = NULL;
		return -errno;
	}

	return firmwared_answer(conn, msg, "%s%s", COMMAND_NAME, list);
}

static const struct command config_keys_command = {
		.name = COMMAND_NAME,
		.help = "Lists all the config keys available.",
		.synopsis = "",
		.handler = config_keys_command_handler,
};

static __attribute__((constructor)) void config_keys_cmd_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&config_keys_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void config_keys_cmd_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(COMMAND_NAME);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
