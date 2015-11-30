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

static int config_keys_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	char __attribute__((cleanup(ut_string_free))) *list = NULL;

	list = config_list_keys();
	if (list == NULL) {
		list = NULL;
		return -errno;
	}

	return firmwared_answer(conn, FWD_ANSWER_CONFIG_KEYS,
			FWD_FORMAT_ANSWER_CONFIG_KEYS, seqnum, list);
}

static const struct command config_keys_command = {
		.msgid = FWD_COMMAND_CONFIG_KEYS,
		.help = "Lists all the config keys available.",
		.synopsis = "",
		.handler = config_keys_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void config_keys_cmd_init(void)
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

	ret = command_unregister(config_keys_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
