THIS FILE IS A DRAFT. Tis is four draft, adding secure savestate commands and signals handling

# Emulator NetworkAccess protocol

The purpose of this protocol is to have a uniform way to communicate with emulators.

See [README.md](README.md) for more general information. This file mostly describes
the protocol itself.

## 1.0 Preview

This is a preview of the 1.0 version of the protocol. The core mechanisms are here but it can
be missing some clarification.


## Primary functions

* Information: Allow to get emulator, core and game information.
* Control: Load game, start/stop/reset emulation.
* Access Memory: Read and write core memories if possible.

## Vocabulary and Context

The protocol separates the notion of emulator and core. An emulator can be a simple single-purpose emulator
like Snes9x/Dolphin or a frontend to multiple platforms like RetroArch or Bizhawk.

A core is often responsible for just the emulation part and a frontend like RetroArch provides the handling of
audio/video and user input.

In this protocol, a regular emulator like Snes9x is an emulator with one core. Something like Bsnes/Higan with
multiple profiles for the same core (accuracy/performance/balanced) may provide a core for each profile.
RetroArch obviously provides multiple cores.

Since emulation and core are often tied, the protocol defines it as CORE-related commands when it's something
specific to a core. As an example, getting the informations about a core is specific to the core, pausing
the emulation is more general.

### Disambiguation

- Emulator: Refers to the whole program, e.g. Snes9x or RetroArch
- Core: The piece of code that handles the plaform emulation, e.g. Bsnes-core
- Emulation: The process of executing the core and thus emulating the platform/game


## TCP Port

The protocol is built on top of TCP.

An emulator implementing this protocol must listen on port 48879 (BEEF in hexadecimal), and if already in use increment by 1.
This exists to support multiple emulators running on the same system.

Implementation of the protocol should also abide by the environment variable `NWA_PORT_RANGE` that allow users to override the starting port

## Client request

A client communicates mostly with basic commands and the emulator will reply according to the command.
The general command syntax is as follow:

```
KEYWORD [ <arguments separated by ';'>]\n
```

### Command Naming

Commands are all upper case ASCII. Multiple words are separated by `_`.

For `LOAD` and `SAVE` the namespace is appended (e.g. `LOAD_GAME`).\
For other commands the namespace is prepended (e.g. `GAME_INFO`).

A command requiering binary data must be prefixed with a `b` lowercase, see later.

### Command Arguments

If arguments are given, The command and the first argument are separated by a single space.
Any additionnals arguments are separated by `;`.

### Number Arguments

Numbers in arguments can be simple decimal without prefix or hexadecimal
with `$` prefix. So `256` is the same as `$100`.

### Binary Transfer to Emulator

Some commands will require the transfer of binary data after sending a command.
The command name must have a lower case 'b' prefix to indicate it expects a binary block.
A binary message follows this format:

```
<0><4 bytes size><data bytes>
```
`size` is encoded in network byte order (big endian).
If a size is given in the command (e.g. for multi-write) the sizes should match.

There can only be 0 or 1 binary transfers per command.

## Emulator Reply

The first byte of the reply indicates its type : `\n` for an ascii reply or `\0` for a binary reply.

An ascii reply ends when 2 consecutive `\n` are received.
A binary reply ends when the size is reached.


### ASCII reply

An ascii reply is represented as an associative array following this format:

```
\n
key:value\n
another_key:another value\n
\n
```

or as a list of an associative array (see last paragraph)
```
\n
name:name1\n
detail:detail1\n
name:name2\n
detail:detail2\n
\n
```

Key names are lower case and separated by _. Values are arbitrary strings and their meaning depends on the command.

For predefined constants (like emulator state) values are in lower case.

Whenever a duplicate key is received this means the reply is a list of associative array.
Example:
```
\n
name:name1\n
detail:detail1\n
name:name2\n
detail:detail2\n
\n
```

This can be translated as this json like data:
```
[ 
{ name : "name1", detail : "detail1"}, 
{ name : "name2", detail : "details2"}
]
```


### Binary reply

A binary reply is the same as a binary message from a client.

```
<0><4 bytes size><data bytes>
```
`size` is encoded in network byte order (big endian) and can differ from what the command requested (e.g. read outside of address range)

### Error reply

An error is a special ascii reply, with an `error` and `reason` key.

```
\n
error:error_type\n
reason:<message>\n
\n
```

Error type are as follow :

* invalid_command : The command is invalid or unsupported by the emulator
* invalid_argument : The arguments for the command are not what the command expect
* not_allowed : The operation is not allowed
* protocol_error : The operation did not respect the protocol

### Success

Any ascii reply that does not contain an `error` key is a success.

The smallest success you can receive from a command is simply an empty reply

```
\n
\n
```

## Handling errors

There are 2 mains type of error. 

-A protocol error is an error in the way message are transmited, etheir is an invalid message, 
eg something not starting with an ascii caracter or a '0' byte or sending something not expected.

-A application level error is specific to a command or what the emulator does not allow, 
eg 'MY_NAME_IS' without an argument is an 'invalid_argument' error.

When a protocol error is encountered, a 'protocol_error' should be sent, the connection is broken and can be closed.


## Mandatory commands

For more details about each commands see after this part.

The following commands must be implemented: `EMULATOR_INFO`, `EMULATION_STATUS`, `CORES_LIST`, `CORE_INFO`, `CORE_CURRENT_INFO`, `MY_NAME_IS`


## Signals

Signals are a way for the emulator to communicate when something happen on its side, like the user loading another game
or reseting the current one. They are not mandatory to implement

They don't cut the flow of regular commands and the client must explicitly register to them.

The emulator must only send signals when it's idling, meaning if a command is currently be processed
you must wait for it to finish before sending the signal.

All keyword related with signals start with a `!`.

### Naming convention

Signals names are verbs, describing the action associed with it, for example a game being loaded will trigger
the signal `!GAME_LOADED`.

### Registration

A client register to a signal by sending the command `!<SIGNAL_NAME>`, you can register to the special `!ALL` signal to
received all signals.

The command `EMULATOR_INFO` list the signals available to the client.

### Signal emition

When a signal occurs the emulator send the signal name prefixed by a `!`, followed by a space and the source of the signal, then a `\n`.
The source value is etheir `USER` if it's from a user interaction with the emulator or a value like `NWA:client_name` if it's from
a command done via this protocol.

This is followed by an ascii reply to provide more informations about the event. If not, the emulator still
need to repy with an empty ascii reply.

### Example

For a `reset` signal triggered by something in the Gui of the emulator (shortcut, menu...)

```
!EMULATION_RESET USER\n
\n
\n
```

```
!GAME_LOADED NWA:control_client\n
\n
... The same reply as the GAME_INFO command
\n
```


## Commands

### MY_NAME_IS <client name>

This command allows the client to specifiy its name. This is useful if the emulator wants to identify what is connecting
to itself. This should always return a success and should probably lead to a display on the emulator (if possible)
  
This returns the client name, note that the emulator is free to change the name passed if 2 clients share the same name.
  
```
name:<client name>
```

### EMULATOR_INFO

Gives information about the emulator

```
name:<name>
version:<version>
nwa_verion:<version of the protocol supported>
id:<an id to identify the running instance of the emulator>
commands:<string separated by commas of the implemented commands>
signals:<string separated by commas of all the signals>
user_defined...
```

`name`, `version`, `nwa_version`, `id`,  and `commands` are mandatory.

The id is needed to identify the emulator since users can start multiples instances of the same emulator on the same system.
It does not need to be a number, something like "Happy Rabbit" is good since it easier for the user to make sens of it.

The `commands` field allows the client to know what is supported by the emulator.

The `signals` field allows the client to know what the signal the emulator can send.

`nwa_version` is a version string representing the version of the protocol implemented following a simple Major.Minor format, eg `"1.0"` 


### EMULATION_STATUS

Returns the state of the emulation.

```
state:<running|paused|stopped|no_game>
game:<game_id>
```
`game_id` is mandatory when the state is running or paused. It can be a name, filename or hash. 
It may be used by the client to detect that the loaded game has changed.

### EMULATION_PAUSE, EMULATION_STOP, EMULATION_RESET, EMULATION_RESUME, EMULATION_RELOAD

Change emulator state.

EMULATION_STOP: console power off.\
EMULATION_RESET: console soft reset.\
EMULATION_RELOAD: reinserting the cartridge/CD if possible or STOP followed by RESUME otherwise.

### LOAD_GAME `<path/to/game.rom>`

Load game from file.

### GAME_INFO

Get information for the loaded game. Keys/values may be platform specific.

```
name:<game_name>
file:<path/to/game.rom>
region:<game_region>
type:<rom_type>
```

### CORES_LIST [`<platform>`]

List of supported cores. If platform is not empty, filter by platform.
```
name:<core_name1>
platform:<platform_name1>
name:<core_name2>
platform:<platform_name2>
```

Name always come first and is the separator key.

### CORE_INFO `<core_name>`

Give information about a core.

```
platform:<plateform_name>
name:<core_name>
version:<core_version>
file:<path/to/core.dll>
```

`platform` and `name` can not be empty.

### CORE_CURRENT_INFO

Give information about the currently loaded core. See CORE_INFO.

### LOAD_CORE `<core_name>`

Load the specified core.

If `<core_name>` is empty, unload core if applicable.

### CORE_RESET

CORE_RESET resets the core to its initial state.

### CORE_MEMORIES

Get information about the available memory of the loaded core.

`name` is defined per platform, see [platforms/](platform documentation).\
`access` is one of `rw`,`r`,`w` for read/write, read-only, write-only.\
`size` is mandatory.

Example:
```
name:WRAM
access:rw
size:131072
name:SRAM
access:rw
size:2048
```

Be careful that these value can be changed between 2 games for the same core. For example SNES games
have differents SRAM size.

A size of 0 can mean the memory is currently unavailable or unknow, like if no game is loaded the rom size is likely 0.

### CORE_READ `<memory_name>` [`<offset>` [`<size>` [`<offset2>` `<size2>` ....]]]

Read one or more ranges from a memory. The emulator will send a binary reply.

Offsets/addresses that start with $ are hexadecimal, otherwise they are decimal.

* If offset is empty, read entire memory.
* If size is empty, read from offset to end.
* If offsetN is given, sizeN can not be omitted for N>=2
* If reading out of bounds for last offset/size: reply size can be shortened (reply has less bytes than requested).
* If reading out of bounds for non-last offset/size: the command error.
* If one address is out of bounds the command returns an error.

Note : for a multiread you still has only one binary reply

Sample: `CORE_READ WRAM;$100;10;512;10` reads 10 bytes from 0x100 and 10 bytes from 0x200 of WRAM. The reply will be 20 bytes long.

### bCORE_WRITE `<memory name>` [`<offset>` [`<size>` [`<offset2>` `<size2>` ....]]]

Write one or more ranges to a memory. The command is followed by binary message.

Offsets/addresses that start with $ are hexadecimal otherwise they are decimal.

* If offset is empty, start write at 0
* If size is empty, use size of binary data.
* If offsetN is given, sizeN can not be omitted for N>=2
* If size is shorter than binary data: reply error
* If size is longer than binary data: reply error

Sample: `CORE_WRITE WRAM;$100;10;512;10` `<20 byte binary block>` writes 10 bytes to 0x100 and 10 bytes to 0x200 of WRAM.

### DEBUG_BREAK

If supported by the emulator, should behave like hitting a breakpoint.

### DEBUG_CONTINUE

If supported by the emulator, should behave like the "continue" button when inside a breakpoint.

### LIST_STATES

```
filename:savestate1.svt
filename:savestate2.svt
```

List the available savestates for the running game. Filenames are relatives paths to where the emulator
stores the savestates for the game. Note that the emulator can also just returns slotnames, as long as
it can be used by the following 2 commands.

### LOAD_STATE `<filename>`

Load a savestate from filename. The filename is a relative path from where the emulator stores
its savestates.

### SAVE_STATE `<filename>`

Make a savestate and store to filename, see LOAD_STATE for constraints.

### bLOAD_STATE_FROM_NETWORK

Load a savestate using the following binary message datas as datas for the savestate.

### SAVE_STATE_TO_NETWORK

Create a savestate and send the datas to the client.

