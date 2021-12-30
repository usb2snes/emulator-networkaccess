# Emulator NetworkAccess protocol

The purpose of this protocol is to have a uniform way to communicate with emulators.

See [README.md](README.md) for more general information. This file mostly describe
the protocol itself.

This is a draft. Feedback is welcome.


## Primary functions

* Information : Allow to get emulator, core and game information.
* Control : Load game, start/stop/reset emulation.
* Access Memory : Read and write core memories if possible.

## Vocabulary and Context

The protocol separate the notion of emulator and core. An emulator can be an simple single purpose emulator
like Snes9x/Dolphin or frontend to multiple platform like RetroArch or Bizhawk.

A core is often responsible for only the emulation part and a frontend like RetroArch provide the handling of
audio/video and user input.

For the protocol purpose, a regular emulator like Snes9x is an emulator with one core. Something like Bsnes/Higan with
multiple profils for one core (accuracy/performance/balanced) provide a core for each profil. RetroArch obviously
provide multiple core.

Since emulation and core are often tied, the protocol tend to only offer CORE related commands when it's something
specific to a core. To try to be more clear, getting the informations about a core is specific to the core, pausing
the emulation is more general

### Keywords

Emulator : Refer to the software itself, IE : Snes9x
Core : The piece of code that handle the plaform emulation eg: Bsnes-core
Emulation : The emulation of a game


## TCP Port

The protocol is build on top of TCP.

An emulator implementing this protocol must listen on port 65400, and if already in use increment by 1.

## Client request

A client will mostly communicate with basic commands. The general command syntax is as follow:

```
KEYWORD[ <arguments separated by ';'>]\n
```

### Command Naming

Commands are all upper case ASCII. Multiple words are separated by `_`.

For `LOAD` and `SAVE` the namespace is appended (e.g. `LOAD_GAME`).\
For other commands the namespace is prepended (e.g. `GAME_INFO`).

### Command Arguments

If arguments are given, commands and the first argument are separated by a single space.
Any additionnals arguments are separated by `;`.

### Number Arguments

Numbers in arguments can be simple decimal without prefix or hexadecimal
with `$` prefix. So `256` is the same as `$100`.

### Binary Transfer to Emulator

Some commands will require the transfer of binary data after sending a command.
```
<0><4 bytes size><data bytes>
```
`size` is encoded in network byte order (big endian).
If a size is given in the command (e.g. for multi-write) the sizes should match.

**TODO** : is there a block for each write on a multiwrite or it's a block with the whole data?

## Emulator Reply

The first byte of the reply indicates its type : `\n` for an ascii reply or `\0` for a binary reply.

An ascii reply ends when 2 consecutive `\n` are received.
A binary reply ends when the size is reached.

### ASCII reply

A ascii reply is represented as a hash map, with keys and values.

```
\n
key:value\n
key1:value2\n
\n
```

A key name is lower case and separated with _. Value format is free since its depends on its meanings.

For predefined values (like emulator state) these values are in lower case.

Keys can repeat, the data then represents a `list<map<string,string>>`.

Whenever a duplicate key is received, a new map is started in the list.

**TODO** : Clarify this.

### Binary reply

A binary reply is the same as a binary request.

```
<0><4 bytes size><data bytes>
```
`size` is encoded in network byte order (big endian) and can differ from what the command requested (e.g. read outside of address range)

### Error reply

An error is a special ascii reply, with an `error` key.

```
\n
error:<message>\n
\n
```

### Success

Any ascii reply that does not contain an `error` key is a success.

If a command does not request data or information, the emulator must still send a `\n\n` message.


### Empty listings

```
\n
\n
```

A regular success is an empty list.

## Mandatory commands

For more details about each commands see after this part.

The following commands must be implemented: `EMULATOR_INFO`, `EMULATION_STATUS`, `CORES_LIST`, `CORE_INFO`, `CORE_CURRENT_INFO`

## Commands

### EMULATOR_INFO

Gives information about the emulator

```
name:<name>
version:<version>
id:<a id to identify the running instance of the emulator>
commands:<string separated by commas of the implemented commands>
user_defined...
```

`name`, `version`, `id` and `commands` are mandatory. The id is needed to identify the emulator since users can start multiples instances
of the same emulator on the same system. The commands allow the client to know what is supported by the emulator.


### EMULATION_STATUS

Returns the state of the emulation.

```
state:<running|paused|stopped|no_game>
game:<game_id>
```
`game_id` is mandatory when the state is running or paused. It can be a name, filename or hash. 
It mostly for the client to detect that the loaded game has changed.

### EMULATION_PAUSE, EMULATION_STOP, EMULATION_RESET, EMULATION_RESUME, EMULATION_RELOAD

Change emulator state.

EMU_STOP : console power off.\
EMU_RESET : console soft reset.\
EMU_RELOAD : reinserting the cartridge/CD if possible or STOP followed by RESUME otherwise.

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

If `<core_name>` is empty, unload core.

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

### CORE_READ `<memory_name>` [`<offset>` [`<size>` [`<offset2>` `<size2>` ....]]]

Read one or more ranges from a memory. The emulator will send a binary reply.

Offsets/addresses that start with $ are hexadecimal, otherwise they are decimal.

* If offset is empty, read entire memory.
* If size is empty, read from offset to end.
* If reading out of bounds for last offset/size: reply can be short (reply has less bytes than requested).
* If reading out of bounds for non-last offset/size: reply has to be padded with 0.
* If **all** addresses are out of bounds the reply can be empty instead of sending padding.
* If offsetN is given, sizeN can not be omitted for N>=2

Sample: `CORE_READ WRAM;$100;10;512;$a` reads 10 bytes from 0x100 and 10 bytes from 0x200 of WRAM. The reply will be 20 bytes long.

### CORE_WRITE `<memory name>` [`<offset>` [`<size>` [`<offset2>` `<size2>` ....]]]

Write one or more ranges to a memory followed by binary data.

Offsets/addresses that start with $ are hexadecimal otherwise they are decimal.

* If offset is empty, start write at 0
* If size is empty, use size of binary data.
* If size is shorter than binary data: reply error
* If size is longer than binary data: reply error
* If offsetN is given, sizeN can not be omitted for N>=2

Sample: `CORE_WRITE WRAM;$100;10;512;$a` `<20 byte binary block>` writes 10 bytes to 0x100 and 10 bytes to 0x200 of WRAM.

### DEBUG_BREAK

If supported by the emulator, should behave like hitting a breakpoint.

### DEBUG_CONTINUE

If supported by the emulator, should behave like the "continue" button when inside a breakpoint.

### LOAD_STATE `<filename>`

Load a savestate from filename.

If the emulator only supports "quick" (not arbitrary filename) savestates,
arbitrary filename support has to be added or left unsupported.

There is no advantage to exposing "quick" savestates interface.

### SAVE_STATE `<filename>`

Make a savestate and store to filename, see LOAD_STATE for constraints.