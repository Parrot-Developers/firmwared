/**
 * @file commands.h
 * @brief
 *
 * @date Apr 17, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <libpomp.h>

#include "firmwared.h"

struct command {
	const char *name;
	const char *help;
	const char *long_help;
	const char *synopsis;
	int (*handler)(struct firmwared *f, struct pomp_conn *conn,
			const struct pomp_msg *msg);
	char *help_msg;
};

const char *command_get_help(const char *name);
int command_invoke(struct firmwared *f, struct pomp_conn *conn,
		const struct pomp_msg *msg);
int command_register(const struct command *cmd);
int command_unregister(const char *name);
const char *command_list(void);

#endif /* COMMANDS_H_ */
