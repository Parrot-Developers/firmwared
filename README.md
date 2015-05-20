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
  list all the items in the folder FOLDER  
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
  launches an instance, which switches to the *STARTED* state
* *KILL* INSTANCE\_IDENTIFIER  
  kills an instance, all the processes are killed, the instance is still
  registered and it's rw aufs layer is still present  
  the instance must be in the *STARTED* state..  
  the instance switches to the *STOPPING* state, before switching back to the
  *READY* state
* *HELP* COMMAND  
  sends back a little help on a given command
* *QUIT*  
  asks firmwared to exit
* *VERSION*  
  sends back informations concerning this firmwared program's version
* *COMMANDS*  
  asks the server to list the currently registered commands

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
environment variables (see config.c for more information):

* *FIRMWARED\_MOUNT\_HOOK*: path to the helper executable responsible of
  mounting the union fs of an instance and of cleaning it, defaults to
  **/usr/libexec/firmwared/mount.hook**
* *FIRMWARED\_SOCKET\_PATH*: where the control socket will be created,
  defaults to **/var/run/firmwared.sock**
* *FIRMWARED\_RESOURCES\_DIR*: where the resource files for firmwared are
  stored, defaults to **/usr/share/firmwared/**
* *FIRMWARED\_REPOSITORY\_PATH*: where the installed firmwares are located,
  defaults to **/usr/share/firmwared/firmwares/**
* *FIRMWARED\_MOUNT\_PATH*: where the firmwares mountings will take place,
  under this directory the following subtree will be used :

                  +- /instance_sha1/
                                   |
                                   +- ro (read-only mount point of the firmware)
                                   +- rw (directory with the rw layer)
                                   +- union (union fs mount of ./rw upon ./ro)
  defaults to **/var/run/firmwared/mount\_points**
* *FIRMWARED\_NET\_HOOK*: path to the helper executable responsible of creating
  the veth pair and configuring it, defaults to
  **/usr/libexec/firmwared/net.hook**

These environment variables are defined in hooks:

* *FIRMWARED\_PREVENT\_REMOVAL*: if set to "y", after dropping an instance, it's
  run artifacts will be preserved, that is, the
  FIRMWARED\_MOUNT\_PATH/instance\_sha1 directory will _not_ be destroyed

The firmwares initial indexing performs sha1 computation which can take a
significant amount of time. Openmp is used to automatically make them in
parallel. It will try to choose the best number of threads automatically.
Limiting their number can be done with:

* *OMP\_NUM\_THREADS*: a number which sets the maximum number of threads, openmp
  will use to compute the firmwares sha1 in parallel. This number doens't take
  the main thread into account.
