# firmwared

## Overview

Daemon responsible of spawning drone firmware instances in containers.  
This document is in markdown format and is best read with a markdown viewer. In
command line, one can use `pandoc README.md | lynx -stdin`.

## Protocol

### Vocabulary

*Commands* are sent by clients. No acknowledgement is sent by the server. The
server sends *notifications*, which can be in reaction to a client *command*.  
Each *notification*'s first argument after the *command* name, is the *pomp msg
id* of the command it is related to, if relevant.  
An *identifier* is either the sha1 of the entity or it's friendly random name.
There is 3 types of entities, "instances", "firmwares" and "remote_firmwares",
entities types are named *folders*.

### Commands

* *PING*
  asks for the server to answer with a *PONG* notification  
* *LIST* FOLDER  
  list all the items in the folder FOLDER  
  FOLDER is one of "instances", "firmwares" or "remote_firmwares"
* *SHOW* FOLDER IDENTIFIER  
  asks for all the information on a given entity of a folder
* *PULL* FIRMWARE_IDENTIFIER  
  installs locally a firmware present on the remote firmwares pool
* *DROP* REMOTE_FIRMWARE_IDENTIFIER  
  removes an remote firmware installed locally
* *PREPARE* FIRMWARE_IDENTIFIER  
  creates an instance of the given firmware, in the *READY* state
* *START* INSTANCE_IDENTIFIER  
  launches an instance, which switches to the *STARTED* state
* *KILL* INSTANCE_IDENTIFIER  
  kills an instance, all the processes are killed, the instance is still
  registered and it's rw aufs layer is still present  
  the instance must be in the *STARTED* state..
  the instance switches to the *STOPPING* state, before switching back to the
  *READY* state
* *REAP* INSTANCE_IDENTIFIER  
  unregister an instance and remove it's rw aufs layer  
  the instance must be in the *STOPPED* state
* *HELP* COMMAND
  sends back a little help on a given command

### Notifications

There are two types of notifications, unicast (marked as "answer to an XXX
command) and broadcast (marked as "notification in reaction to an XXX command).

* *PONG* CID  
  answer to a *PING*
* *LIST* CID FOLDER [list of (ID, NAME) pairs]  
  answer to a *LIST* command
* *SHOW* CID FOLDER ID NAME INFORMATION_STRING  
  answer to a *SHOW* command. The actual content of the INFORMATION_STRING is
  dependent on the FOLDER queried and is for display purpose
* *PULLED* CID FIRMWARE_ID FIRMWARE_NAME  
  notification in reaction to a *PULL* command
* *DROPPED* CID FIRMWARE_ID FIRMWARE_NAME  
  notification in reaction to a *DROP* command
* *STARTED* CID FIRMWARE_ID FIRMWARE_NAME INSTANCE_ID INSTANCE_NAME  
  notification in reaction to a *START* command
* *STOPPED* CID FIRMWARE_ID FIRMWARE_NAME INSTANCE_ID INSTANCE_NAME  
  notification in reaction to a *STOP* command
* *REAPED* CID FIRMWARE_ID FIRMWARE_NAME INSTANCE_ID INSTANCE_NAME  
  notification in reaction to a *REAP* command
* *HELP* CID COMMAND HELP_TEXT
  answer to a *REAP* command
* *ERROR* CID ERRNO MESSAGE
  answer to any command whose execution encountered a problem

### Instance states

* *READY*
  an instance of a firmware has been created and is ready to be started  
  this state is reach after a *PREPARE* command on a firmware, or after a *KILL*
  command, issued on a *STARTED* instance
* *STARTED*  
  the instance is currently running, that is, it's pid 1 is alive  
  this state is reached after a *START* command, issued on a STOPPED instance
* *STOPPING*
  transitional state, reached by an instance after a *KILL* command has been
  issued  
  soon after, the firmware should reach the *READY* state

## Implementation details

### Error handling

The general rule of thumb is :

 * most of the time, the return value is for error reporting, a negative errno-
 compatible value is returned on error, while 0 indicates success.
 * if a pointer is returned then a NULL return indicates an error iif errno is
 set to non-zero
