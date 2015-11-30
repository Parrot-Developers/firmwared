/**
 * @file fwd.c
 * @brief
 *
 * @date 24 nov. 2015
 * @author ncarrier
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <ut_string.h>

#include <fwd.h>

/* set an interpreter section so that this lib can be ran as an executable */
const char fwd_interp[] __attribute__((section(".interp"))) = FWD_INTERPRETER;

#define FWD_MESSAGE_INVALID_STR "(invalid)"

/*
 * associates with a command, the answer indicating it has completed
 * successfully
 * @note this array passes through a sed command to generate the corresponding
 * mapping for the fdc shell script, so, changes to it's layout must be thought
 * of carefully and precisely reflected in the fdc.answers generation command
 */
static const enum fwd_message fwd_command_answer_pair[] = {
		[FWD_COMMAND_COMMANDS] =     FWD_ANSWER_COMMANDS,
		[FWD_COMMAND_CONFIG_KEYS] =  FWD_ANSWER_CONFIG_KEYS,
		[FWD_COMMAND_DROP] =         FWD_ANSWER_DROPPED,
		[FWD_COMMAND_FOLDERS] =      FWD_ANSWER_FOLDERS,
		[FWD_COMMAND_GET_CONFIG] =   FWD_ANSWER_GET_CONFIG,
		[FWD_COMMAND_GET_PROPERTY] = FWD_ANSWER_GET_PROPERTY,
		[FWD_COMMAND_HELP] =         FWD_ANSWER_HELP,
		[FWD_COMMAND_KILL] =         FWD_ANSWER_DEAD,
		[FWD_COMMAND_LIST] =         FWD_ANSWER_LIST,
		[FWD_COMMAND_PING] =         FWD_ANSWER_PONG,
		[FWD_COMMAND_PREPARE] =      FWD_ANSWER_PREPARED,
		[FWD_COMMAND_PROPERTIES] =   FWD_ANSWER_PROPERTIES,
		[FWD_COMMAND_QUIT] =         FWD_ANSWER_BYEBYE,
		[FWD_COMMAND_REMOUNT] =      FWD_ANSWER_REMOUNTED,
		[FWD_COMMAND_SET_PROPERTY] = FWD_ANSWER_PROPERTY_SET,
		[FWD_COMMAND_SHOW] =         FWD_ANSWER_SHOW,
		[FWD_COMMAND_START] =        FWD_ANSWER_STARTED,
		[FWD_COMMAND_VERSION] =      FWD_ANSWER_VERSION,
};

const char *fwd_message_str(enum fwd_message message)
{
	switch (message) {
	/* commands, i.e. from client to server */
	case FWD_COMMAND_COMMANDS:
		return "COMMANDS";
	case FWD_COMMAND_CONFIG_KEYS:
		return "CONFIG_KEYS";
	case FWD_COMMAND_DROP:
		return "DROP";
	case FWD_COMMAND_FOLDERS:
		return "FOLDERS";
	case FWD_COMMAND_GET_CONFIG:
		return "GET_CONFIG";
	case FWD_COMMAND_GET_PROPERTY:
		return "GET_PROPERTY";
	case FWD_COMMAND_HELP:
		return "HELP";
	case FWD_COMMAND_KILL:
		return "KILL";
	case FWD_COMMAND_LIST:
		return "LIST";
	case FWD_COMMAND_PING:
		return "PING";
	case FWD_COMMAND_PREPARE:
		return "PREPARE";
	case FWD_COMMAND_PROPERTIES:
		return "PROPERTIES";
	case FWD_COMMAND_QUIT:
		return "QUIT";
	case FWD_COMMAND_REMOUNT:
		return "REMOUNT";
	case FWD_COMMAND_SET_PROPERTY:
		return "SET_PROPERTY";
	case FWD_COMMAND_SHOW:
		return "SHOW";
	case FWD_COMMAND_START:
		return "START";
	case FWD_COMMAND_VERSION:
		return "VERSION";
	/* answers, i.e. from server to client */
	/* acks */
	case FWD_ANSWER_COMMANDS:
		return "COMMANDS";
	case FWD_ANSWER_CONFIG_KEYS:
		return "CONFIG_KEYS";
	case FWD_ANSWER_ERROR:
		return "ERROR";
	case FWD_ANSWER_FOLDERS:
		return "FOLDERS";
	case FWD_ANSWER_GET_CONFIG:
		return "GET_CONFIG";
	case FWD_ANSWER_GET_PROPERTY:
		return "GET_PROPERTY";
	case FWD_ANSWER_HELP:
		return "HELP";
	case FWD_ANSWER_LIST:
		return "LIST";
	case FWD_ANSWER_PONG:
		return "PONG";
	case FWD_ANSWER_PROPERTIES:
		return "PROPERTIES";
	case FWD_ANSWER_PROPERTY_SET:
		return "PROPERTY_SET";
	case FWD_ANSWER_REMOUNTED:
		return "REMOUNTED";
	case FWD_ANSWER_SHOW:
		return "SHOW";
	case FWD_ANSWER_VERSION:
		return "VERSION";
	/* notifications */
	case FWD_ANSWER_BYEBYE:
		return "BYEBYE";
	case FWD_ANSWER_DEAD:
		return "DEAD";
	case FWD_ANSWER_DROPPED:
		return "DROPPED";
	case FWD_ANSWER_PREPARED:
		return "PREPARED";
	case FWD_ANSWER_PREPARE_PROGRESS:
		return "PREPARE_PROGRESS";
	case FWD_ANSWER_STARTED:
		return "STARTED";

	default:
		return FWD_MESSAGE_INVALID_STR;
	}
}

const char *fwd_message_format(enum fwd_message message)
{
	switch (message) {
	/* commands, i.e. from client to server */
	case FWD_COMMAND_COMMANDS:
		return FWD_FORMAT_COMMAND_COMMANDS;
	case FWD_COMMAND_CONFIG_KEYS:
		return FWD_FORMAT_COMMAND_CONFIG_KEYS;
	case FWD_COMMAND_DROP:
		return FWD_FORMAT_COMMAND_DROP;
	case FWD_COMMAND_FOLDERS:
		return FWD_FORMAT_COMMAND_FOLDERS;
	case FWD_COMMAND_GET_CONFIG:
		return FWD_FORMAT_COMMAND_GET_CONFIG;
	case FWD_COMMAND_GET_PROPERTY:
		return FWD_FORMAT_COMMAND_GET_PROPERTY;
	case FWD_COMMAND_HELP:
		return FWD_FORMAT_COMMAND_HELP;
	case FWD_COMMAND_KILL:
		return FWD_FORMAT_COMMAND_KILL;
	case FWD_COMMAND_LIST:
		return FWD_FORMAT_COMMAND_LIST;
	case FWD_COMMAND_PING:
		return FWD_FORMAT_COMMAND_PING;
	case FWD_COMMAND_PREPARE:
		return FWD_FORMAT_COMMAND_PREPARE;
	case FWD_COMMAND_PROPERTIES:
		return FWD_FORMAT_COMMAND_PROPERTIES;
	case FWD_COMMAND_QUIT:
		return FWD_FORMAT_COMMAND_QUIT;
	case FWD_COMMAND_REMOUNT:
		return FWD_FORMAT_COMMAND_REMOUNT;
	case FWD_COMMAND_SET_PROPERTY:
		return FWD_FORMAT_COMMAND_SET_PROPERTY;
	case FWD_COMMAND_SHOW:
		return FWD_FORMAT_COMMAND_SHOW;
	case FWD_COMMAND_START:
		return FWD_FORMAT_COMMAND_START;
	case FWD_COMMAND_VERSION:
		return FWD_FORMAT_COMMAND_VERSION;
	/* answers, i.e. from server to client */
	/* acks */
	case FWD_ANSWER_COMMANDS:
		return FWD_FORMAT_ANSWER_COMMANDS;
	case FWD_ANSWER_CONFIG_KEYS:
		return FWD_FORMAT_ANSWER_CONFIG_KEYS;
	case FWD_ANSWER_ERROR:
		return FWD_FORMAT_ANSWER_ERROR;
	case FWD_ANSWER_FOLDERS:
		return FWD_FORMAT_ANSWER_FOLDERS;
	case FWD_ANSWER_GET_CONFIG:
		return FWD_FORMAT_ANSWER_GET_CONFIG;
	case FWD_ANSWER_GET_PROPERTY:
		return FWD_FORMAT_ANSWER_GET_PROPERTY;
	case FWD_ANSWER_HELP:
		return FWD_FORMAT_ANSWER_HELP;
	case FWD_ANSWER_LIST:
		return FWD_FORMAT_ANSWER_LIST;
	case FWD_ANSWER_PONG:
		return FWD_FORMAT_ANSWER_PONG;
	case FWD_ANSWER_PROPERTIES:
		return FWD_FORMAT_ANSWER_PROPERTIES;
	case FWD_ANSWER_PROPERTY_SET:
		return FWD_FORMAT_ANSWER_PROPERTY_SET;
	case FWD_ANSWER_REMOUNTED:
		return FWD_FORMAT_ANSWER_REMOUNTED;
	case FWD_ANSWER_SHOW:
		return FWD_FORMAT_ANSWER_SHOW;
	case FWD_ANSWER_VERSION:
		return FWD_FORMAT_ANSWER_VERSION;
	/* notifications */
	case FWD_ANSWER_BYEBYE:
		return FWD_FORMAT_ANSWER_BYEBYE;
	case FWD_ANSWER_DEAD:
		return FWD_FORMAT_ANSWER_DEAD;
	case FWD_ANSWER_DROPPED:
		return FWD_FORMAT_ANSWER_DROPPED;
	case FWD_ANSWER_PREPARED:
		return FWD_FORMAT_ANSWER_PREPARED;
	case FWD_ANSWER_PREPARE_PROGRESS:
		return FWD_FORMAT_ANSWER_PREPARE_PROGRESS;
	case FWD_ANSWER_STARTED:
		return FWD_FORMAT_ANSWER_STARTED;

	default:
		return FWD_FORMAT_INVALID;
	}
}

enum fwd_message fwd_message_from_str(const char *str)
{
	enum fwd_message m;

	for (m = FWD_MESSAGE_FIRST; m < FWD_MESSAGE_LAST; m++)
		if (ut_string_match(fwd_message_str(m), str))
			return m;

	return FWD_MESSAGE_INVALID;
}

bool fwd_message_is_invalid(enum fwd_message message)
{
	return ut_string_match(fwd_message_str(message),
			FWD_MESSAGE_INVALID_STR);
}

enum fwd_message fwd_message_command_answer(enum fwd_message command)
{
	if (fwd_message_is_invalid(command))
		return FWD_MESSAGE_INVALID;

	if (command > FWD_COMMAND_LAST)
		return FWD_MESSAGE_INVALID;

	return fwd_command_answer_pair[command];
}

const char libfwd_usage[] = "usage: [LIBFWD_GET_ANSWER_ID=] "
		"LIBFWD_MESSAGE=MESSAGE_NAME libfwd.so\n"
		"\tIf LIBFWD_GET_ANSWER_ID is defined, outputs the answer id a "
		"client must wait for, in order to know if the command "
		"MESSAGE_NAME was successful, otherwise, outputs the command's "
		"id.";

void libfwd_main(void)
{
	const char *str_message = getenv("LIBFWD_MESSAGE");
	enum fwd_message message = fwd_message_from_str(str_message);
	enum fwd_message result;

	/* without this, strange things do happen */
	setlinebuf(stdout);
	if (getenv("LIBFWD_HELP") != NULL) {
		puts(libfwd_usage);
		_exit(EXIT_SUCCESS);
	}

	if (getenv("LIBFWD_GET_MESSAGE_FORMAT") != NULL) {
		puts(fwd_message_format(message));
	} else {
		if (getenv("LIBFWD_GET_ANSWER_ID") != NULL)
			result = fwd_message_command_answer(message);
		else
			result = message;
		printf("%d\n", result);
	}

	_exit(EXIT_SUCCESS);
}
