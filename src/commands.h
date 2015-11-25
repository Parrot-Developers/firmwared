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

#include <fwd.h>

#include "firmwared.h"

#define COMMAND_CONSTRUCTOR_PRIORITY (FIRMWARED_CONSTRUCTOR_PRIORITY + 1)

struct command {
	enum fwd_message msgid;
	const char *help;
	const char *long_help;
	const char *synopsis;
	int (*handler)(struct pomp_conn *conn, const struct pomp_msg *msg,
			uint32_t seqnum);
	char *help_msg;
};

const char *command_get_help(enum fwd_message msgid);
int command_invoke(struct pomp_conn *conn, const struct pomp_msg *msg);
int command_register(const struct command *cmd);
int command_unregister(enum fwd_message msgid);
const char *command_list(void);

#endif /* COMMANDS_H_ */
