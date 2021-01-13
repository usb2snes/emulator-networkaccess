# Network Access emulator protocol

## General

<blabla> the main purpose of this protocol is to have an unified way to access different component of an emulator and also be able to control.

## Main functions

Allow to get core and game informations
Load game, stop the emulation, reset the emulation
List all core memory available to be able to read/write them


## Starting port

65400

## Command format

`KEYWORD<space>[list of args with ;]\n`

## Emulator Reply

hash like data :

```
key:value\n
key1:value2\n
\n\n
```

A repeat of the first key is a list.

binary data transfert to client :
Success :

`<0><4 bytes size><datas>`

error :
 `<1> error:blablabla\n\n`


`status:OK\n\n` for success of commands

```
status:NOK\n
error:my little error\n
\n```


empty replies are `none:none`

## Commands

### EMU_INFOS

Gives information about the emulator (name, version, etc...)

### EMU_STATUS

Returns what state the emulator is.

```
state:<running|paused|nogame>
```

### CORES_LIST <opt>

List of supported cores, if given a plateform, list the core for this plateform

### CORE_INFO <core name>

Give information about the core.

```
plateform:plateform name
name:core name

```

### CURRENT_CORE_INFO

Give information about the loaded core

### LOAD_CORE <core name>

Load the specified core

### LOAD_GAME <filepath>

Load the file

### EMU_PAUSE, EMU_STOP, EMU_RESET

### CORE_MEMORIES

Get information about the available memory of the loaded core

```
name:WRAM
access:rw
name:SRAM
access:rw
```

### READ_CORE_MEMORY <memory name> <offset> <size> [offset2 size2, ....]

Read a part of the memory

### WRITE_CORE_MEMORY <memory name> <offset> <size> [offset2, size2, ....]

Then send the data
