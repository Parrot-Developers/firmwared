# firmwared

## Overview

Daemon responsible of spawning drone firmware instances in containers.  
This document is in markdown format and is best read with a markdown viewer. In
command line, one can use `pandoc README.md | lynx -stdin`.

## Quick start

Instructions start from the root of a sphinx workspace. First unzip the example
firmware.

        $ tar xf packages/firmwared/examples/example_firmware.ext2.tar.bz2

Move it to the firmware repository path, *firmwared* is configured to use:

        $ mv example_firmware.ext2 firmwares/example_firmware.ext2.firmware

Start firmwared :

        # ULOG_LEVEL=D ULOG_STDERR=y ULOG_STDERR_COLOR=y firmwared packages/firmwared/examples/firmwared.conf

Use the example startup script :

        # ./packages/firmwared/examples/launch_instance.sh

An xterm should popup, with a console opened on the running instance's console.

## Protocol

### Vocabulary

*Commands* are sent by clients.  The server sends *answers*, which can be
in reaction to a client *command* but not only.  
An *identifier* is either the sha1 of the entity or it's friendly random name.
As the time of writing, there is 2 types of entities, "instances", "firmwares",
entities types are named *folders*.

### Commands

* *ADD\_PROPERTY* FOLDER NAME  
  adds a custom property to the folder FOLDER. If name ends with [], the
  property will be an array. Custom properties are all mutable.
* *COMMANDS*  
  asks the server to list the currently registered commands
* *CONFIG_KEYS*  
  lists all the available configuration keys.
* *DROP* FOLDER IDENTIFIER  
  removes an entity from a folder  
  if the entity is an instance, it must be in the *READY* state. It's pid 1 will
  be killed and it's run artifacts will be removed if
  FIRMWARED\_PREVENT\_REMOVAL isn't set to "y"
* *FOLDERS*  
  asks the server to list the currently registered folders
* *GET\_CONFIG* CONFIG_KEY  
  retrieves the value of the CONFIG_KEY configuration key.
* *GET\_PROPERTY* FOLDER ENTITY\_IDENTIFIER PROPERTY\_NAME  
  retrieves the value of the property PROPERTY for the entity whose name or sha1
  is ENTITY\_IDENTIFIER from the folder FOLDER.
* *HELP* COMMAND  
  sends back a little help on the command COMMAND
* *KILL* INSTANCE\_IDENTIFIER  
  kills an instance, all the processes are killed, the instance is still
  registered and it's rw aufs layer is still present  
  the instance must be in the *STARTED* state.  
  the instance switches to the *STOPPING* state, before switching back to the
  *READY* state
* *LIST* FOLDER  
  lists all the items in the folder FOLDER  
  FOLDER is one of folders listed in an answer to a *FOLDERS* command
* *PING*  
  asks for the server to answer with a *PONG* notification
* *PREPARE* FOLDER IDENTIFICATION\_STRING  
  prepares either a firmware or an instance.  
  If FOLDER is equal to "firmwares", then a firmware will be prepared.
  In this case, IDENTIFICATION\_STRING can be either:
   1. a path to an ext2 file system image:  
     in this case, the file will be copied in the firmware repository, indexed
     by firmwared and listed as a usable firmware
   1. an URL:  
     in this case, the file corresponding to the URL will be downloaded locally
     in the firmware repository, indexed by firmwared and listed as a usable
     firmware.
   1. a path to a directory:
     in this case, the path will be considered to correspond to a final
     directory and will be indexed by firmwared and listed as a usable
     firmware.  
  If FOLDER is equal to "instances", then an instance will be created for the 
  firmware of identifier IDENTIFICATION\_STRING, in the *READY* state  
  IDENTIFICATION\_STRING must correspond to an identifier of a registered
  firmware.
* *PROPERTIES* FOLDER  
  asks the server to list the currently registered properties for the folder
  FOLDER
* *QUIT*  
  asks firmwared to exit
* *REMOUNT* INSTANCE\_IDENTIFIER  
  asks to remount the union file system of an instance, to take into account
  modifications in the lower dir (e.g. rebuild of a final dir)
* *SET\_PROPERTY* FOLDER ENTITY\_IDENTIFIER PROPERTY\_NAME PROPERTY\_VALUE  
  sets the value of the property PROPERTY to the value PROPERTY\_VALUE, for the
  entity whose name or sha1 is ENTITY\_IDENTIFIER from the folder FOLDER.
* *SHOW* FOLDER IDENTIFIER  
  asks for all the information on a given entity of a folder
* *START* INSTANCE\_IDENTIFIER  
  launches an instance, which switches to the *STARTED* state and must be in the
  READY state
* *VERSION*  
  sends back informations concerning this firmwared program's version

### Answers

*Answers* are of two types: *acks*, unicast answer to the client which issued a
command and *notifications*, broadcast to all the clients currently listening.  

#### Acks

* *PROPERTY\_ADDED* FOLDER PROPERTY  
  answer to an *ADD\_PROPERTY* command  
* *COMMANDS* LIST  
  answer to a *COMMANDS* command, LIST is a space-separated list of the commands
  implemented in firmwared  
* *CONFIG\_KEYS* CONFIG\_KEYS\_LIST  
  answer to a *CONFIG\_KEYS* command, CONFIG\_KEYS\_LIST is a space-separated
  list of the configuration keys available in firmwared
* *ERROR* ERRNO MESSAGE  
  answer to any command whose execution encountered a problem
* *FOLDERS* FOLDERS\_LIST  
  answer to a *FOLDERS* command, FOLDERS\_LIST is a space-separated list of the
  folders registered so far
* *GET\_CONFIG* CONFIG\_KEY CONFIG\_VALUE  
  answer to a *GET\_CONFIG* command
* *GET\_PROPERTY* FOLDER ENTITY\_IDENTIFIER PROPERTY\_NAME PROPERTY\_VALUE  
  answer to a *GET\_PROPERTY* command
* *HELP* COMMAND HELP\_TEXT  
  answer to a *HELP* command
* *LIST* FOLDER COUNT [list of (ID, NAME) pairs]  
  answer to a *LIST* command
* *PONG*  
  answer to a *PING*
* *SHOW* FOLDER ID NAME INFORMATION\_STRING  
  answer to a *SHOW* command. The actual content of the INFORMATION\_STRING is
  dependent on the FOLDER queried and is for display purpose
* *VERSION* VERSION\_DESCRIPTION  
  answer to a *VERSION* command.
* *PROPERTIES* FOLDER PROPERTIES\_LIST  
  answer to a *PROPERTIES* command, PROPERTIES\_LIST is a space-separated list
  of the properties registered for the folder
* *PROPERTY\_SET* FOLDER ENTITY\_IDENTIFIER PROPERTY\_NAME PROPERTY\_VALUE  
  answer to a *SET\_PROPERTY* command
* *REMOUNTED* INSTANCE\_IDENTIFIER  
  answer to a *REMOUNT* command

#### Notifications

* *BYEBYE*  
  notification in reaction to the reception of a *QUIT* command
* *DEAD* INSTANCE\_ID INSTANCE\_NAME  
  notification in reaction to the end of an instance's main process, be it
  caused by a *KILL* command or by "natural death"
* *DROPPED* FOLDER ENTITY\_ID ENTITY\_NAME  
  notification in reaction to a *DROP* command
* *PREPARED* FOLDER ENTITY\_ID ENTITY\_NAME  
  notification in reaction to a *PREPARE* command
* *PREPARE\_PROGRESS* FOLDER IDENTIFICATION\_STRING PROGRESS  
  notification in reaction to a *PREPARE* command, indicating the progression of
  the preparation. PROGRESS is a percentage.
* *STARTED* INSTANCE\_ID INSTANCE\_NAME  
  notification in reaction to a *START* command

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
