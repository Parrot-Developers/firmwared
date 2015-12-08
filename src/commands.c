/**
 * @file commands.c
 * @brief
 *
 * @date Apr 17, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <ut_string.h>

#include "commands.h"

#define ULOG_TAG firmwared_commands
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_commands);

#define COMMANDS_MAX (FWD_COMMAND_LAST + 1)

static struct command commands[COMMANDS_MAX];

static char *list;

static struct command *command_find(enum fwd_message msgid)
{
	int i;

	for (i = 0; i < COMMANDS_MAX; i++)
		if (commands[i].msgid == msgid)
			return commands + i;

	return NULL;
}

static bool command_is_invalid(const struct command *cmd)
{
	return cmd == NULL || cmd->help == NULL ||
			fwd_message_is_invalid(cmd->msgid) ||
			cmd->handler == NULL || cmd->synopsis == NULL;
}

static void command_dump(const struct command *cmd)
{
	ULOGD("\t%s: \"%s\"", fwd_message_str(cmd->msgid), cmd->help);
}

static int command_process(struct pomp_conn *conn,
		const struct pomp_msg *msg, uint32_t seqnum)
{
	const struct command *cmd;
	uint32_t msgid;

	msgid = pomp_msg_get_id(msg);
	cmd = command_find(msgid);
	if (cmd == NULL) {
		ULOGE("command \"%"PRIu32"\" isn't implemented", msgid);
		return -ENOSYS;
	}

	ULOGD("execute command %s", fwd_message_str(cmd->msgid));

	return cmd->handler(conn, msg, seqnum);
}

static int command_build_help_message(struct command *cmd)
{
	int ret;
	const char *name;

	name = fwd_message_str(cmd->msgid);

	ret = asprintf(&cmd->help_msg, "Command %s\n"
			"Synopsis: %s %s\n"
			"Overview: %s\n%s", name, name, cmd->synopsis,
			cmd->help, cmd->long_help ? cmd->long_help : "");
	if (ret < 0) {
		cmd->help_msg = NULL;
		ULOGE("asprintf error");
		return -ENOMEM;
	}

	return 0;
}
const char *command_get_help(enum fwd_message msg)
{
	int ret;
	struct command *command;

	errno = EINVAL;
	if (fwd_message_is_invalid(msg))
		return NULL;

	command = command_find(msg);
	if (command == NULL) {
		ULOGE("command_find: %s not found", fwd_message_str(msg));
		errno = ESRCH;
		return NULL;
	}

	if (command->help_msg == NULL) {
		ret = command_build_help_message(command);
		if (ret < 0)
			errno = -ret;
	}

	return command->help_msg;
}

int command_invoke(struct pomp_conn *conn, const struct pomp_msg *msg)
{
	int ret;
	uint32_t seqnum;

	ret = pomp_msg_read(msg, "%"PRIu32, &seqnum);
	if (ret < 0) {
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}
	ret = command_process(conn, msg, seqnum);
	if (ret < 0) {
		ULOGE("command_process: %s", strerror(-ret));
		return firmwared_answer(conn, FWD_ANSWER_ERROR,
				FWD_FORMAT_ANSWER_ERROR, seqnum, -ret,
				strerror(-ret));
	}

	return 0;
}

int command_register(const struct command *cmd)
{
	const struct command *needle;
	int i;

	if (command_is_invalid(cmd))
		return -EINVAL;

	/* command name must be unique */
	needle = command_find(cmd->msgid);
	if (needle != NULL)
		return -EEXIST;

	/* find first free slot */
	for (i = 0; i < COMMANDS_MAX; i++)
		if (command_is_invalid(commands + i))
			break;

	if (i >= COMMANDS_MAX)
		return -ENOMEM;

	commands[i] = *cmd;

	return 0;
}

int command_unregister(enum fwd_message msgid)
{
	struct command *needle;
	struct command *max = commands + COMMANDS_MAX - 1;

	needle = command_find(msgid);
	if (needle == NULL)
		return -ESRCH;
	ut_string_free(&needle->help_msg);

	for (; needle < max; needle++)
		*needle = *(needle + 1);
	memset(max - 1, 0, sizeof(*needle)); /* NULL guard */

	return 0;
}

const char *command_list(void)
{
	int ret;
	struct command *command = commands + COMMANDS_MAX;

	/* the result is cached */
	if (list != NULL)
		return list;

	while (command-- > commands) {
		if (command_is_invalid(command))
			continue;
		ret = ut_string_append(&list, "%s ",
				fwd_message_str(command->msgid));
		if (ret < 0) {
			ULOGC("ut_string_append");
			errno = -ret;
			return NULL;
		}
	}
	if (list == NULL) {
		ULOGE("no command registered, that _is_ weird");
		errno = ENOENT;
		return NULL;
	}
	if (list[0] != '\0')
		list[strlen(list) - 1] = '\0';

	return list;
}

static __attribute__((destructor)) void commands_cleanup(void)
{
	ut_string_free(&list);
}
