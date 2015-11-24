/**
 * @file fwd.c
 * @brief
 *
 * @date 24 nov. 2015
 * @author ncarrier
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <fwd.h>

const char *fmw_message_str(enum fwd_message message)
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
	case FWD_ANSWER_ERROR:
		return "ERROR";
	case FWD_ANSWER_FOLDERS:
		return "FOLDERS";
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
	}

	return "(invalid)";
}
