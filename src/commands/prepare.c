/**
 * @file prepare.c
 * @brief
 *
 * @date Apr 22, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <openssl/sha.h>

#include <ut_string.h>

#include <libpomp.h>

#define ULOG_TAG firmwared_command_prepare
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_command_prepare);

#include "commands.h"
#include "utils.h"
#include "firmwares.h"
#include "instances.h"
#include "folders.h"
#include "utils.h"

static int prepare_command_handler(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	int ret;
	char __attribute__((cleanup(ut_string_free))) *folder = NULL;
	char __attribute__((cleanup(ut_string_free))) *identification_string =
			NULL;

	/* coverity[bad_printf_format_string] */
	ret = pomp_msg_read(msg, FWD_FORMAT_COMMAND_PREPARE_READ, &seqnum, &folder,
			&identification_string);
	if (ret < 0) {
		folder = identification_string = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	return folder_prepare(folder, identification_string, seqnum);
}

static const struct command prepare_command = {
		.msgid = FWD_COMMAND_PREPARE,
		.help = "Creates an instance from a firmware, in the READY "
				"state, of create a firmware from an URL, a "
				"path to a final directory or a path to an "
				"ext2 image of a firmware.",
		.long_help = "If FOLDER equals to firmware, then "
				"IDENTIFICATION_STRING can be a path or an url "
				"in this case, the corresponding firmware will "
				"be retrieved using curl. "
				"It can also be a path to a final folder, a "
				"firmware will then be registered from this "
				"directory.\n"
				"If FOLDER equals to instance, then "
				"IDENTIFICATION_STRING must be either a sha1 "
				"or a friendly name of a previously registered "
				"firmware. "
				"A new instance will then be created and "
				"registered from this firmware.",
		.synopsis = "FOLDER IDENTIFICATION_STRING",
		.handler = prepare_command_handler,
};

static __attribute__((constructor(COMMAND_CONSTRUCTOR_PRIORITY)))
		void prepare_init(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_register(&prepare_command);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}

static __attribute__((destructor)) void prepare_cleanup(void)
{
	int ret;

	ULOGD("%s", __func__);

	ret = command_unregister(prepare_command.msgid);
	if (ret < 0)
		ULOGE("command_register: %s", strerror(-ret));
}
