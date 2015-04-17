# firmwared

## Overview

Daemon responsible of spawning drone firmware instances in containers.

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
* *START* FIRMWARE_IDENTIFIER  
  launches an instance of the given firmware
* *KILL* INSTANCE_IDENTIFIER  
  kills an instance, all the processes are killed, the instance is still
  registered and it's rw aufs layer is still present
* *REAP* INSTANCE_IDENTIFIER  
  unregister an instance and remove it's rw aufs layer
* *HELP* COMMAND
  sends back a little help on a given command

### Notifications

* *PONG* CID  
  answer to a *PING*
* *LIST* CID FOLDER [list of (ID, NAME) pairs]  
  answer to a *LIST* command
* *SHOW* CID FOLDER ID NAME INFORMATION_STRING  
  answer to a *SHOW* command. The actual content of the INFORMATION_STRING is
  dependent on the FOLDER queried and is for display purpose
* *PULLED* CID FIRMWARE_ID FIRMWARE_NAME  
  answer to a *PULL* command
* *DROPPED* CID FIRMWARE_ID FIRMWARE_NAME  
  answer to a *DROP* command
* *STARTED* CID FIRMWARE_ID FIRMWARE_NAME INSTANCE_ID INSTANCE_NAME  
  answer to a *START* command
* *STOPPED* CID FIRMWARE_ID FIRMWARE_NAME INSTANCE_ID INSTANCE_NAME  
  answer to a *STOP* command
* *REAPED* CID FIRMWARE_ID FIRMWARE_NAME INSTANCE_ID INSTANCE_NAME  
  answer to a *REAP* command
* *HELP* CID COMMAND HELP_TEXT
  answer to a *REAP* command
