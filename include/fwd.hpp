/**
 * @file fwd.hpp
 * @brief
 *
 * @date 3 déc. 2015
 * @author ncarrier
 * @copyright Copyright (C) 2015 Parrot S.A.
 */

#ifndef INCLUDE_FWD_HPP_
#define INCLUDE_FWD_HPP_

#include <libpomp.hpp>

#include "fwd.h"

typedef pomp::MessageFormat<FWD_COMMAND_COMMANDS, pomp::ArgU32> MsgFmtCommandCommands;
typedef pomp::MessageFormat<FWD_COMMAND_CONFIG_KEYS, pomp::ArgU32> MsgFmtCommandConfigKeys;
typedef pomp::MessageFormat<FWD_COMMAND_DROP, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr> MsgFmtCommandDrop;
typedef pomp::MessageFormat<FWD_COMMAND_FOLDERS, pomp::ArgU32> MsgFmtCommandFolders;
typedef pomp::MessageFormat<FWD_COMMAND_GET_CONFIG, pomp::ArgU32, pomp::ArgStr> MsgFmtCommandGetConfig;
typedef pomp::MessageFormat<FWD_COMMAND_GET_PROPERTY, pomp::ArgU32,
                pomp::ArgStr, pomp::ArgStr, pomp::ArgStr> MsgFmtCommandGetProperty;
typedef pomp::MessageFormat<FWD_COMMAND_HELP, pomp::ArgU32, pomp::ArgStr> MsgFmtCommandHelp;
typedef pomp::MessageFormat<FWD_COMMAND_KILL, pomp::ArgU32, pomp::ArgStr> MsgFmtCommandKill;
typedef pomp::MessageFormat<FWD_COMMAND_LIST, pomp::ArgU32, pomp::ArgStr> MsgFmtCommandList;
typedef pomp::MessageFormat<FWD_COMMAND_PING, pomp::ArgU32> MsgFmtCommandPing;
typedef pomp::MessageFormat<FWD_COMMAND_PREPARE, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr> MsgFmtCommandPrepare;
typedef pomp::MessageFormat<FWD_COMMAND_PROPERTIES, pomp::ArgU32, pomp::ArgStr> MsgFmtCommandProperties;
typedef pomp::MessageFormat<FWD_COMMAND_QUIT, pomp::ArgU32> MsgFmtCommandQuit;
typedef pomp::MessageFormat<FWD_COMMAND_REMOUNT, pomp::ArgU32, pomp::ArgStr> MsgFmtCommandRemount;
typedef pomp::MessageFormat<FWD_COMMAND_SET_PROPERTY, pomp::ArgU32,
                pomp::ArgStr, pomp::ArgStr, pomp::ArgStr, pomp::ArgStr> MsgFmtCommandSetProperty;
typedef pomp::MessageFormat<FWD_COMMAND_SHOW, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr> MsgFmtCommandShow;
typedef pomp::MessageFormat<FWD_COMMAND_START, pomp::ArgU32, pomp::ArgStr> MsgFmtCommandStart;
typedef pomp::MessageFormat<FWD_COMMAND_VERSION, pomp::ArgU32> MsgFmtCommandVersion;

typedef pomp::MessageFormat<FWD_ANSWER_COMMANDS, pomp::ArgU32, pomp::ArgStr> MsgFmtAnswerCommands;
typedef pomp::MessageFormat<FWD_ANSWER_CONFIG_KEYS, pomp::ArgU32, pomp::ArgStr> MsgFmtAnswerConfigKeys;
typedef pomp::MessageFormat<FWD_ANSWER_ERROR, pomp::ArgU32, pomp::ArgI32,
                pomp::ArgStr> MsgFmtAnswerError;
typedef pomp::MessageFormat<FWD_ANSWER_FOLDERS, pomp::ArgU32, pomp::ArgStr> MsgFmtAnswerFolders;
typedef pomp::MessageFormat<FWD_ANSWER_GET_CONFIG, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr> MsgFmtAnswerGetConfig;
typedef pomp::MessageFormat<FWD_ANSWER_GET_PROPERTY, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr, pomp::ArgStr, pomp::ArgStr> MsgFmtAnswerGetProperty;
typedef pomp::MessageFormat<FWD_ANSWER_HELP, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr> MsgFmtAnswerHelp;
typedef pomp::MessageFormat<FWD_ANSWER_LIST, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgU32, pomp::ArgStr> MsgFmtAnswerList;
typedef pomp::MessageFormat<FWD_ANSWER_PONG, pomp::ArgU32> MsgFmtAnswerPong;
typedef pomp::MessageFormat<FWD_ANSWER_PROPERTIES, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr> MsgFmtAnswerProperties;
typedef pomp::MessageFormat<FWD_ANSWER_PROPERTY_SET, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr, pomp::ArgStr, pomp::ArgStr> MsgFmtAnswerPropertySet;
typedef pomp::MessageFormat<FWD_ANSWER_REMOUNTED, pomp::ArgU32> MsgFmtAnswerRemounted;
typedef pomp::MessageFormat<FWD_ANSWER_SHOW, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr, pomp::ArgStr, pomp::ArgStr> MsgFmtAnswerShow;
typedef pomp::MessageFormat<FWD_ANSWER_VERSION, pomp::ArgU32, pomp::ArgStr> MsgFmtAnswerVersion;
typedef pomp::MessageFormat<FWD_ANSWER_BYEBYE, pomp::ArgU32> MsgFmtAnswerByebye;
typedef pomp::MessageFormat<FWD_ANSWER_DEAD, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr> MsgFmtAnswerDead;
typedef pomp::MessageFormat<FWD_ANSWER_DROPPED, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr, pomp::ArgStr> MsgFmtAnswerDropped;
typedef pomp::MessageFormat<FWD_ANSWER_PREPARED, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr, pomp::ArgStr> MsgFmtAnswerPrepared;
typedef pomp::MessageFormat<FWD_ANSWER_PREPARE_PROGRESS, pomp::ArgU32,
                pomp::ArgStr, pomp::ArgStr, pomp::ArgStr> MsgFmtAnswerPrepareProgress;
typedef pomp::MessageFormat<FWD_ANSWER_STARTED, pomp::ArgU32, pomp::ArgStr,
                pomp::ArgStr> MsgFmtAnswerStarted;

#endif /* INCLUDE_FWD_HPP_ */
