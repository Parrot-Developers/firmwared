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

struct command {
	const char *name;
	const char *help;
	int (*handler)(const struct pomp_msg *msg); // TODO
};

int command_invoke(struct pomp_decoder *dec, const struct pomp_msg *msg);
int command_register(const struct command *cmd);
int command_unregister(const char *name);
void command_list(void);

#endif /* COMMANDS_H_ */
