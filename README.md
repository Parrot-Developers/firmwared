# firmwared

## Overview

Daemon responsible of spawning drone firmware instances in containers.  
This document is in markdown format and is best read with a markdown viewer. In
command line, one can use `pandoc README.md | lynx -stdin`.

## Protocol

### Vocabulary

*Commands* are sent by clients. No acknowledgement is sent by the server. The
server sends *notifications*, which can be in reaction to a client *command*.  
An *identifier* is either the sha1 of the entity or it's friendly random name.
As the time of writing, there is 3 types of entities, "instances", "firmwares"
and "remote\_firmwares", entities types are named *folders*.

### Commands

* *PING*  
  asks for the server to answer with a *PONG* notification
* *FOLDERS*  
  asks the server to list the currently registered folders
* *LIST* FOLDER  
  lists all the items in the folder FOLDER  
  FOLDER is one of folders listed in an answer to a *FOLDERS* command
* *SHOW* FOLDER IDENTIFIER  
  asks for all the information on a given entity of a folder
* *PULL* REMOTE\_FIRMWARE_IDENTIFIER  
  installs locally a firmware present on the remote firmwares pool
* *DROP* FOLDER IDENTIFIER  
  removes an entity from a folder  
  if the entity is an instance, it must be in the *READY* state. It's pid 1 will
  be killed and it's run artifacts will be removed if
  FIRMWARED\_PREVENT\_REMOVAL isn't set to "y"
* *PREPARE* FIRMWARE\_IDENTIFIER  
  creates an instance of the given firmware, in the *READY* state  
  if FIRMWARE\_IDENTIFIER doesn't correspond to a registered firmware, then it
  is supposed to be a path to a directory which will be mounted as the read-only
  layer for the prepared instance, this allows to create instances from the
  final directory of a firmware's workspace and is intended for development.
* *START* INSTANCE\_IDENTIFIER  
  launches an instance, which switches to the *STARTED* state and must be in the
  READY state
* *KILL* INSTANCE\_IDENTIFIER  
  kills an instance, all the processes are killed, the instance is still
  registered and it's rw aufs layer is still present  
  the instance must be in the *STARTED* state.  
  the instance switches to the *STOPPING* state, before switching back to the
  *READY* state
* *HELP* COMMAND  
  sends back a little help on the command COMMAND
* *QUIT*  
  asks firmwared to exit
* *VERSION*  
  sends back informations concerning this firmwared program's version
* *COMMANDS*  
  asks the server to list the currently registered commands
* *PROPERTIES* FOLDER  
  asks the server to list the currently registered properties for the folder
  FOLDER
* *GET\_PROPERTY* FOLDER ENTITY\_IDENTIFIER PROPERTY\_NAME  
  retrieves the value of the property PROPERTY for the entity whose name or sha1
  is ENTITY\_IDENTIFIER from the folder FOLDER.
* *SET\_PROPERTY* FOLDER ENTITY\_IDENTIFIER PROPERTY\_NAME PROPERTY\_VALUE  
  sets the value of the property PROPERTY to the value PROPERTY\_VALUE, for the
  entity whose name or sha1 is ENTITY\_IDENTIFIER from the folder FOLDER.

### Notifications

There are two types of notifications, unicast (marked as "answer to an XXX
command) and broadcast (marked as "notification in reaction to an XXX command).

* *PONG*  
  answer to a *PING*
* *FOLDERS* FOLDERS\_LIST  
  answer to a *FOLDERS* command, FOLDERS\_LIST is a comma-separated list of the
  folders registered so far
* *LIST* FOLDER COUNT [list of (ID, NAME) pairs]  
  answer to a *LIST* command
* *SHOW* FOLDER ID NAME INFORMATION\_STRING  
  answer to a *SHOW* command. The actual content of the INFORMATION\_STRING is
  dependent on the FOLDER queried and is for display purpose
* *PULLED* FIRMWARE\_ID FIRMWARE\_NAME  
  notification in reaction to a *PULL* command
* *DROPPED* FOLDER ENTITY\_ID ENTITY\_NAME  
  notification in reaction to a *DROP* command
* *PREPARED* FIRMWARE\_ID FIRMWARE\_PATH INSTANCE\_ID INSTANCE\_NAME  
  notification in reaction to a *PREPARE* command
* *STARTED* INSTANCE\_ID INSTANCE\_NAME  
  notification in reaction to a *START* command
* *DEAD* INSTANCE\_ID INSTANCE\_NAME  
  notification in reaction to the end of an instance's main process, be it
  caused by a *KILL* command or by "natural death"
* *HELP* COMMAND HELP\_TEXT  
  answer to a *HELP* command
* *VERSION* VERSION\_DESCRIPTION  
  answer to a *VERSION* command.
* *BYEBYE*  
  acknowledge reception of a *QUIT* command
* *ERROR* ERRNO MESSAGE  
  answer to any command whose execution encountered a problem
* *PROPERTIES* FOLDER PROPERTIES\_LIST  
  answer to a *PROPERTIES* command, PROPERTIES\_LIST is a comma-separated list
  of the properties registered for the folder
* *GET\_PROPERTY* FOLDER ENTITY\_IDENTIFIER PROPERTY\_NAME PROPERTY\_VALUE  
  answer to a *GET\_PROPERTY* command
* *PROPERTY\_SET* FOLDER ENTITY\_IDENTIFIER PROPERTY\_NAME PROPERTY\_VALUE  
  answer to a *SET\_PROPERTY* command

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

## Environment variables

Some base configuration parameters can be modified at startup via the following
environment variables (see config.c for more information). All the supported
variables are documented in the *firmwared*(1) manpage.

## Configuration file

firmwared accepts an optional path to a configuration file as it's first
argument. The syntax is that of the lua language, an example is provided in
examples/firmwared.conf. The admitted keys are the same as the FIRMWARED_XXX
environment variables.