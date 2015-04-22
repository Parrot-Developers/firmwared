/**
 * @file commands.c
 * @brief
 *
 * @date Apr 17, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "commands.h"

#define ULOG_TAG firmwared_commands
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_commands);

#define COMMANDS_MAX 20

static struct command commands[COMMANDS_MAX];

static struct command *command_find(const char *name)
{
	int i;

	for (i = 0; i < COMMANDS_MAX; i++)
		if (commands[i].name == NULL)
			return NULL;
		else if (strcmp(name, commands[i].name) == 0)
			return commands + i;

	return NULL;
}

static bool str_is_invalid(const char *str)
{
	return str == NULL || *str == '\0';
}

static bool command_is_invalid(const struct command *cmd)
{
	return cmd == NULL || cmd->help == NULL || str_is_invalid(cmd->name) ||
				cmd->handler == NULL;
}

static void command_dump(const struct command *cmd)
{
	ULOGD("\t%s: \"%s\"", cmd->name, cmd->help);
}

int command_invoke(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg)
{
	int ret;
	char *name = NULL;
	const struct command *cmd;

	pomp_decoder_init(f->decoder, msg);
	ret = pomp_decoder_read_str(f->decoder, &name);
	pomp_decoder_clear(f->decoder);
	if (ret < 0) {
		ULOGE("pomp_decoder_read_str: %s", strerror(-ret));
		return ret;
	}

	cmd = command_find(name);
	if (cmd == NULL) {
		ULOGE("command \"%s\" isn't implemented", name);
		return -ENOSYS;
	}

	ret = cmd->handler(f, conn, msg);
	if (ret < 0)
		pomp_conn_send(conn, firmwared_get_msg_id(f), "%s%"PRIu32"%d%s",
			"ERROR", pomp_msg_get_id(msg), -ret, strerror(-ret));

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
	if (needle != NULL)
		return -EEXIST;

	for (; needle < max; needle++)
		*needle = *(needle + 1);
	memset(needle + 1, 0, sizeof(*needle)); /* NULL guard */

	return 0;
}

void command_list(void)
{
	int i;
	const struct command *cmd;

	ULOGD("Registered commands so far, are :");
	for (i = 0; i < COMMANDS_MAX ; i++) {
		cmd = commands + i;
		if (cmd->name == NULL)
			return;
		command_dump(cmd);
	}
}
