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

#define COMMANDS_MAX 20

static struct command commands[COMMANDS_MAX];

static char *list;

static struct command *command_find(const char *name)
{
	int i;

	for (i = 0; i < COMMANDS_MAX; i++)
		if (commands[i].name == NULL)
			return NULL;
		else if (strcasecmp(name, commands[i].name) == 0)
			return commands + i;

	return NULL;
}

static bool command_is_invalid(const struct command *cmd)
{
	return cmd == NULL || cmd->help == NULL ||
			ut_string_is_invalid(cmd->name) ||
			cmd->handler == NULL || cmd->synopsis == NULL;
}

static void command_dump(const struct command *cmd)
{
	ULOGD("\t%s: \"%s\"", cmd->name, cmd->help);
}

static int command_process(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char __attribute__((cleanup(ut_string_free)))*name = NULL;
	const struct command *cmd;

	ret = pomp_msg_read(msg, "%ms", &name);
	if (ret < 0) {
		cmd = NULL;
		ULOGE("pomp_msg_read: %s", strerror(-ret));
		return ret;
	}

	cmd = command_find(name);
	if (cmd == NULL) {
		ULOGE("command \"%s\" isn't implemented", name);
		return -ENOSYS;
	}

	ULOGD("execute command %s", name);

	return cmd->handler(f, conn, msg);
}

static int command_build_help_message(struct command *command)
{
	int ret;

	ret = asprintf(&command->help_msg, "Command %s\n"
			"Synopsis: %s %s\n"
			"Overview: %s\n%s", command->name, command->name,
			command->synopsis, command->help, command->long_help ?
					command->long_help : "");
	if (ret < 0) {
		ULOGE("asprintf error");
		return -ENOMEM;
	}

	return 0;
}
const char *command_get_help(const char *name)
{
	int ret;
	struct command *command;

	errno = EINVAL;
	if (ut_string_is_invalid(name))
		return NULL;

	command = command_find(name);
	if (command == NULL) {
		ULOGE("command_find: %s not found", name);
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

int command_invoke(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;

	ret = command_process(f, conn, msg);
	if (ret < 0) {
		ULOGE("command_process: %s", strerror(-ret));
		return firmwared_answer(conn, msg, "%s%d%s", "ERROR", -ret,
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
	needle = command_find(cmd->name);
	if (needle != NULL)
		return -EEXIST;

	/* find first free slot */
	for (i = 0; i < COMMANDS_MAX; i++)
		if (commands[i].name == NULL)
			break;

	if (i >= COMMANDS_MAX)
		return -ENOMEM;

	commands[i] = *cmd;

	return 0;
}

int command_unregister(const char *name)
{
	struct command *needle;
	struct command *max = commands + COMMANDS_MAX - 1;

	needle = command_find(name);
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
		if (command->name == NULL)
			continue;
		ret = ut_string_append(&list, "%s ", command->name);
		if (ret < 0) {
			ULOGC("ut_string_append");
			errno = -ret;
			return NULL;
		}
	}
	if (list[0] != '\0')
		list[strlen(list) - 1] = '\0';

	return list;
}

static __attribute((destructor)) void commands_cleanup(void)
{
	ut_string_free(&list);
}
