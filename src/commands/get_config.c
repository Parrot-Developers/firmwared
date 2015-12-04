/**
 * @file get_config.c
 * @brief
 *
 * @date June 12, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <ut_string.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_get_config
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_get_config);

#include "config.h"
#include "firmwared.h"
#include "commands.h"

static int get_config_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *config_key = NULL;
	char *prefixed_config_key = NULL;
	enum config_key key;
	size_t len;

	ret = pomp_msg_read(msg, FWD_FORMAT_COMMAND_GET_CONFIG_READ, &seqnum,
			&config_key);
	if (ret < 0) {
		config_key = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}
	len = UT_ARRAY_SIZE(CONFIG_KEYS_PREFIX) + strlen(config_key);
	prefixed_config_key = alloca(len);
	snprintf(prefixed_config_key, len, CONFIG_KEYS_PREFIX"%s", config_key);
	key = config_key_from_string(prefixed_config_key);
	if (key == (enum config_key)-1)
		return -ESRCH;

	/* coverity[bad_printf_format_string] */
	return firmwared_answer(conn, FWD_ANSWER_GET_CONFIG,
			FWD_FORMAT_ANSWER_GET_CONFIG, seqnum, config_key,
			config_get(key));
}

static const struct command get_config_command = {
		.msgid = FWD_COMMAND_GET_CONFIG,
		.help = "Retrieves the value of the CONFIG_KEY configuration "
				"key.",
		.long_help = "The CONFIG_KEY is case insensitive. Use the "
				"CONFIG_KEYS command to list the available "
				"config keys to query.",
		.synopsis = "CONFIG_KEY",
		.handler = get_config_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void get_config_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&get_config_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void get_config_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(get_config_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
