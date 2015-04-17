/**
 * @file commands.c
 * @brief
 *
 * @date Apr 17, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>

#include "commands.h"

#define ULOG_TAG firmwared_commands
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_commands);

int command_invoke(const struct pomp_msg *msg)
{
	ULOGE("%s STUB !!!", __func__);

	// lookup for the command
	// invoke

	return -ENOSYS;
}

int command_register(const struct command *cmd)
{
	ULOGE("%s STUB !!!", __func__);

	return -ENOSYS;
}

int command_unregister(const char *name)
{
	ULOGE("%s STUB !!!", __func__);

	return -ENOSYS;
}

int command_list(void)
{
	ULOGE("%s STUB !!!", __func__);

	return -ENOSYS;
}
