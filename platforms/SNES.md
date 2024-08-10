# SNES specifics for Emulator NetworkAccess

## Memories

Not all memories have to be available through EmuNWAccess

* CARTROM: ROM inside the cartridge (required)
* SRAM: SRAM inside the cartridge (required, can be 0 length)
* WRAM: "Work" RAM inside the console (required)
* VRAM: Video RAM inside the console
* OAM: Object attribute memory
* CGRAM: Color palette memory
* APURAM: The APU memory
* CPUBUS: The memory bus as seen by the CPU
* APUBUS: The memory bus as seen by the APU
