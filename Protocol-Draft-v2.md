# Emulator NetworkAccess protocol

The purpose of this protocol is to have a uniform way to communicate with emulators.

See [README.md](README.md) for more.

This is a draft. Feedback is welcome.


## Primary functions

* Allow to get core and game information
* Load game, start/stop/reset emulation
* Read and write core memories


## TCP Port

65400,
if already in use increment by 1.


## Command to Emulator

```
KEYWORD[ <arguments separated by ';'>]\n
```

### Command Naming

Commands are all upper case ASCII. Multiple words are separated by `_`.

For `LOAD` and `SAVE` the namespace is appended (e.g. `LOAD_GAME`).\
For other commands the namespace is prepended (e.g. `GAME_INFO`).

### Command Arguments

If arguments are given, command and arguments are separated by a single space.
Arguments are command-specific, by convention arguments are separated by `;`.

### Number Arguments

Numbers in arguments can bet transferred decimal without prefix or hexadecimal
with `$` prefix. So `256` is the same as `$100`.

### Binary Transfer to Emulator

Some commands will require the transfer of binary data after sending a command.
```
<0><4 bytes size><data bytes>
```
`size` is encoded in network byte order (big endian).
If a size is given in the command (e.g. for multi-write) the sizes should match.

**NOTE:** `<0><size>` was omitted in the original proposal for App -> Emu. \
This would have broken the ability to add binary commands in the future because
the connection would be in an undefined state if data contained `\n`.

## Emulator Reply

First byte indicates if reply is ascii (`\n`) or binary (`\0`).\
Ascii replies end when 2 consecutive `\n` are received.\
Binary replies end when the binary block is complete.

**NOTE:** First `'\n'` was omitted in the original proposal for ascii replies.\
It is not strictly required (we could check for `!= '\0'`), but it allows easy checking for valid reply / protocol errors.

### ASCII data

```
\n
key:value\n
key1:value2\n
\n
```
Keys and predefined values are all lower case, words are separated with _.

Keys can repeat, data then represents a `list<map<string,string>>`.\
Otherwise it represents a `map<string,string>`.\
Whenever a duplicate key is received, a new map is started in the list.

### Binary data

```
<0><4 bytes size><data bytes>
```
`size` is encoded in network byte order (big endian) and can differ from what the command requested (e.g. read outside of address range)

### Error (always ascii)

```
\n
error:<message>\n
\n
```

### Success (always ascii)

any ascii reply that does not contain `error:`, so `\n\n` would be the shortest "success"

**FIXME:** skars wants "ok". What would we gain by this?

### Empty listings

```
\n
none:none\n
\n
```
**FIXME:** we probably would want to return `\n\n` instead, so we could
```
::toMapArray().length() == 0 || !::toMapArray()[0].hasKey("name")
```
to check for empty or incomplete list.


## Commands

### EMU_INFO

Gives information about the emulator
```
name:<name>
version:<version>
user_defined...
```
**NOTE:** renamed from `EMU_INFOS` to `EMU_INFO`.

### EMU_STATUS

Returns what state the emulator is in.

```
state:<running|paused|stopped|no_game>
game:<game_id>
```
`game_id` is optional, can be a name, filename or hash and is just to detect that the loaded game has changed.

### EMU_PAUSE, EMU_STOP, EMU_RESET, EMU_RESUME, EMU_RELOAD

Change emulator state.

EMU_STOP ^= console power off.\
EMU_RESET ^= console soft reset.\
EMU_RELOAD ^= reinserting the cartridge/CD if possible or STOP followed by RESUME otherwise.

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
**FIXME:** when does the next item in the array start?

### CORE_INFO `<core_name>`

Give information about a core.

```
platform:<plateform_name>
name:<core_name>
version:<core_version>
file:<path/to/core.dll>
```
`<core_name>` can not be empty.

### CORE_CURRENT_INFO

Give information about the currently loaded core. See CORE_INFO.

**NOTE:** `CURRENT_CORE_INFO` was renamed to `CORE_CURRENT_INFO`

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

Read one or more ranges from a memory. Emu will send binary reply.

Offsets/addresses that start with $ are hexadecimal, otherwise they are decimal.

* If offset is empty, read entire memory.
* If size is empty, read from offset to end.
* If reading out of bounds for last offset/size: reply can be short (reply has less bytes than requested).
* If reading out of bounds for non-last offset/size: reply has to be padded with 0.
* If **all** addresses are out of bounds the reply can be empty instead of sending padding.
* If offsetN is given, sizeN can not be omitted for N>=2

Sample: `CORE_READ WRAM;$100;10;512;$a` reads 10 bytes from 0x100 and 10 bytes from 0x200 of WRAM. Reply will be 20 bytes long.

### CORE_WRITE `<memory name>` [`<offset>` [`<size>` [`<offset2>` `<size2>` ....]]]

Write one or more ranges to a memory. Followed by binary data.

Offsets/addresses that start with $ are hexadecimal, otherwise they are decimal.

* If offset is empty, start write at 0
* If size is empty, use size of binary data.
* If size is shorter than binary data, ignore extra bytes from binary data or reply error
* If size is longer than binary data, ignore missing bytes from binary data or reply error
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

Make a savestate and store to filename.
