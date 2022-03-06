# Emulator NetworkAccess protocol

The purpose of this protocol is to have a uniform way to communicate with emulators.

See [README.md](README.md) for more general information. This file mostly describes
the protocol itself.

This is a draft. Feedback is welcome.


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

Emulator: Refers to the whole program, e.g. Snes9x or RetroArch
Core: The piece of code that handles the plaform emulation, e.g. Bsnes-core
Emulation: The process of executing the core and thus emulating the platform/game


## TCP Port

The protocol is built on top of TCP.

An emulator implementing this protocol must listen on port 65400, and if already in use increment by 1.

## Client request

A client will mostly communicate with basic commands and the emulator will reply according to the command. The general command syntax is as follow:

```
KEYWORD [ <arguments separated by ';'>]\n
```

### Command Naming

Commands are all upper case ASCII. Multiple words are separated by `_`.

For `LOAD` and `SAVE` the namespace is appended (e.g. `LOAD_GAME`).\
For other commands the namespace is prepended (e.g. `GAME_INFO`).

### Command Arguments

If arguments are given, The command and the first argument are separated by a single space.
Any additionnals arguments are separated by `;`.

### Number Arguments

Numbers in arguments can be simple decimal without prefix or hexadecimal
with `$` prefix. So `256` is the same as `$100`.

### Binary Transfer to Emulator

Some commands will require the transfer of binary data after sending a command.
A binary message follows this format:

```
<0><4 bytes size><data bytes>
```
`size` is encoded in network byte order (big endian).
If a size is given in the command (e.g. for multi-write) the sizes should match.

There can only be 0 or 1 binary transfers per command.\
An unexpected binary transfer is to be ignored by the emulator so that it will correctly receive and discard it
without breaking the stream.

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

### Success

Any ascii reply that does not contain an `error` key is a success.

The smallest succes you can receive from a command is simply an empty reply

```
\n
\n
```


## Mandatory commands

For more details about each commands see after this part.

The following commands must be implemented: `EMULATOR_INFO`, `EMULATION_STATUS`, `CORES_LIST`, `CORE_INFO`, `CORE_CURRENT_INFO`, `MY_NAME_IS`

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
user_defined...
```

`name`, `version`, `nwa_version`, `id`,  and `commands` are mandatory.

The id is needed to identify the emulator since users can start multiples instances of the same emulator on the same system.

The `commands` field allow the client to know what is supported by the emulator.


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

`platform` and `core_name` can not be empty.

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
`size` is optional.

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

### CORE_READ `<memory_name>` [`<offset>` [`<size>` [`<offset2>` `<size2>` ....]]]

Read one or more ranges from a memory. The emulator will send a binary reply.

Offsets/addresses that start with $ are hexadecimal, otherwise they are decimal.

* If offset is empty, read entire memory.
* If size is empty, read from offset to end.
* If offsetN is given, sizeN can not be omitted for N>=2
* If reading out of bounds for last offset/size: reply size can be shortened (reply has less bytes than requested).
* If reading out of bounds for non-last offset/size: the command error.
* If one address is out of bounds the command returns an error.

Note : for a multiread you still receive only one binary reply

Sample: `CORE_READ WRAM;$100;10;512;$a` reads 10 bytes from 0x100 and 10 bytes from 0x200 of WRAM. The reply will be 20 bytes long.

### CORE_WRITE `<memory name>` [`<offset>` [`<size>` [`<offset2>` `<size2>` ....]]]

Write one or more ranges to a memory. The command is followed by binary message.

Offsets/addresses that start with $ are hexadecimal otherwise they are decimal.

* If offset is empty, start write at 0
* If size is empty, use size of binary data.
* If offsetN is given, sizeN can not be omitted for N>=2
* If size is shorter than binary data: reply error
* If size is longer than binary data: reply error

Sample: `CORE_WRITE WRAM;$100;10;512;$a` `<20 byte binary block>` writes 10 bytes to 0x100 and 10 bytes to 0x200 of WRAM.

### DEBUG_BREAK

If supported by the emulator, should behave like hitting a breakpoint.

### DEBUG_CONTINUE

If supported by the emulator, should behave like the "continue" button when inside a breakpoint.

### LOAD_STATE `<filename>`

Load a savestate from filename.

If the emulator only supports "quick" (not an arbitrary filepath) savestates,
arbitrary filename support has to be added or the command left unsupported.

There is no real value to exposing "quick" savestates interface.

### SAVE_STATE `<filename>`

Make a savestate and store to filename, see LOAD_STATE for constraints.
