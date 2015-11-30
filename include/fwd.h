/**
 * @file fwd.h
 * @brief
 *
 * @date 24 nov. 2015
 * @author ncarrier
 * @copyright Copyright (C) 2015 Parrot S.A.
 */

#ifndef INCLUDE_FWD_H_
#define INCLUDE_FWD_H_
#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * printf formats for sending a command of for reading the arguments (with _READ
 * suffix)
 */
#define FWD_FORMAT_COMMAND_COMMANDS "%"PRIu32
#define FWD_FORMAT_COMMAND_CONFIG_KEYS "%"PRIu32
#define FWD_FORMAT_COMMAND_DROP "%"PRIu32"%s%s"
#define FWD_FORMAT_COMMAND_DROP_READ "%"PRIu32"%ms%ms"
#define FWD_FORMAT_COMMAND_FOLDERS "%"PRIu32
#define FWD_FORMAT_COMMAND_GET_CONFIG "%"PRIu32"%s"
#define FWD_FORMAT_COMMAND_GET_CONFIG_READ "%"PRIu32"%ms"
#define FWD_FORMAT_COMMAND_GET_PROPERTY "%"PRIu32"%s%s%s"
#define FWD_FORMAT_COMMAND_GET_PROPERTY_READ "%"PRIu32"%ms%ms%ms"
#define FWD_FORMAT_COMMAND_HELP "%"PRIu32"%s"
#define FWD_FORMAT_COMMAND_HELP_READ "%"PRIu32"%ms"
#define FWD_FORMAT_COMMAND_KILL "%"PRIu32"%s"
#define FWD_FORMAT_COMMAND_KILL_READ "%"PRIu32"%ms"
#define FWD_FORMAT_COMMAND_LIST "%"PRIu32"%s"
#define FWD_FORMAT_COMMAND_LIST_READ "%"PRIu32"%ms"
#define FWD_FORMAT_COMMAND_PING "%"PRIu32
#define FWD_FORMAT_COMMAND_PREPARE "%"PRIu32"%s%s"
#define FWD_FORMAT_COMMAND_PREPARE_READ "%"PRIu32"%ms%ms"
#define FWD_FORMAT_COMMAND_PROPERTIES "%"PRIu32"%s"
#define FWD_FORMAT_COMMAND_PROPERTIES_READ "%"PRIu32"%ms"
#define FWD_FORMAT_COMMAND_QUIT "%"PRIu32
#define FWD_FORMAT_COMMAND_REMOUNT "%"PRIu32"%s"
#define FWD_FORMAT_COMMAND_REMOUNT_READ "%"PRIu32"%ms"
#define FWD_FORMAT_COMMAND_SET_PROPERTY "%"PRIu32"%s%s%s%s"
#define FWD_FORMAT_COMMAND_SET_PROPERTY_READ "%"PRIu32"%ms%ms%ms%ms"
#define FWD_FORMAT_COMMAND_SHOW "%"PRIu32"%s%s"
#define FWD_FORMAT_COMMAND_SHOW_READ "%"PRIu32"%ms%ms"
#define FWD_FORMAT_COMMAND_START "%"PRIu32"%s"
#define FWD_FORMAT_COMMAND_START_READ "%"PRIu32"%ms"
#define FWD_FORMAT_COMMAND_VERSION "%"PRIu32

/*
 * printf formats for sending an answer
 */
#define FWD_FORMAT_ANSWER_COMMANDS "%"PRIu32"%s"
#define FWD_FORMAT_ANSWER_CONFIG_KEYS "%"PRIu32"%s"
#define FWD_FORMAT_ANSWER_ERROR "%"PRIu32"%d%s"
#define FWD_FORMAT_ANSWER_FOLDERS "%"PRIu32"%s"
#define FWD_FORMAT_ANSWER_GET_CONFIG "%"PRIu32"%s%s"
#define FWD_FORMAT_ANSWER_GET_PROPERTY "%"PRIu32"%s%s%s%s"
#define FWD_FORMAT_ANSWER_HELP "%"PRIu32"%s%s"
#define FWD_FORMAT_ANSWER_LIST "%"PRIu32"%s%u%s"
#define FWD_FORMAT_ANSWER_PONG "%"PRIu32
#define FWD_FORMAT_ANSWER_PROPERTIES "%"PRIu32"%s%s"
#define FWD_FORMAT_ANSWER_PROPERTY_SET "%"PRIu32"%s%s%s%s"
#define FWD_FORMAT_ANSWER_REMOUNTED "%"PRIu32
#define FWD_FORMAT_ANSWER_SHOW "%"PRIu32"%s%s%s%s"
#define FWD_FORMAT_ANSWER_VERSION "%"PRIu32"%s"
#define FWD_FORMAT_ANSWER_BYEBYE "%"PRIu32
#define FWD_FORMAT_ANSWER_DEAD "%"PRIu32"%s%s"
#define FWD_FORMAT_ANSWER_DROPPED "%"PRIu32"%s%s%s"
#define FWD_FORMAT_ANSWER_PREPARED "%"PRIu32"%s%s%s"
#define FWD_FORMAT_ANSWER_PREPARE_PROGRESS "%"PRIu32"%s%s%s"
#define FWD_FORMAT_ANSWER_STARTED "%"PRIu32"%s%s"

#define FWD_FORMAT_INVALID ""

/* values must stay consecutive */
enum fwd_message {
	FWD_MESSAGE_INVALID,

	/* commands, i.e. from client to server */
	FWD_MESSAGE_FIRST,
	FWD_COMMAND_FIRST = FWD_MESSAGE_FIRST,

	FWD_COMMAND_COMMANDS = FWD_COMMAND_FIRST,
	FWD_COMMAND_CONFIG_KEYS,
	FWD_COMMAND_DROP,
	FWD_COMMAND_FOLDERS,
	FWD_COMMAND_GET_CONFIG,
	FWD_COMMAND_GET_PROPERTY,
	FWD_COMMAND_HELP,
	FWD_COMMAND_KILL,
	FWD_COMMAND_LIST,
	FWD_COMMAND_PING,
	FWD_COMMAND_PREPARE,
	FWD_COMMAND_PROPERTIES,
	FWD_COMMAND_QUIT,
	FWD_COMMAND_REMOUNT,
	FWD_COMMAND_SET_PROPERTY,
	FWD_COMMAND_SHOW,
	FWD_COMMAND_START,
	FWD_COMMAND_VERSION,

	FWD_COMMAND_LAST = FWD_COMMAND_VERSION,

	/* answers, i.e. from server to client */
	FWD_ANSWER_FIRST,

	/* acks */
	FWD_ANSWER_COMMANDS = FWD_ANSWER_FIRST,
	FWD_ANSWER_CONFIG_KEYS,
	FWD_ANSWER_ERROR,
	FWD_ANSWER_FOLDERS,
	FWD_ANSWER_GET_CONFIG,
	FWD_ANSWER_GET_PROPERTY,
	FWD_ANSWER_HELP,
	FWD_ANSWER_LIST,
	FWD_ANSWER_PONG,
	FWD_ANSWER_PROPERTIES,
	FWD_ANSWER_PROPERTY_SET,
	FWD_ANSWER_REMOUNTED,
	FWD_ANSWER_SHOW,
	FWD_ANSWER_VERSION,

	/* notifications */
	FWD_ANSWER_BYEBYE,
	FWD_ANSWER_DEAD,
	FWD_ANSWER_DROPPED,
	FWD_ANSWER_PREPARED,
	FWD_ANSWER_PREPARE_PROGRESS,
	FWD_ANSWER_STARTED,

	FWD_ANSWER_LAST = FWD_ANSWER_STARTED,
	FWD_MESSAGE_LAST = FWD_ANSWER_LAST,
};

const char *fwd_message_str(enum fwd_message message);

enum fwd_message fwd_message_from_str(const char *str);

bool fwd_message_is_invalid(enum fwd_message message);

/**
 * Allows to retrieve the answer a client must wait for, in order to know if
 * it's command has succeeded.
 * @param command command to retrieve the waited answer for
 * @return answer
 */
enum fwd_message fwd_message_command_answer(enum fwd_message command);

/**
 * Returns the printf format needed by libpomp to send this message
 * @param message message to retrieve the format for
 * @return format corresponding to the message
 */
const char *fwd_message_format(enum fwd_message message);

void libfwd_main(void);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_FWD_H_ */
