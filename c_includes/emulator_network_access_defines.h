#ifndef H_EMULATOR_NETWORK_ACCESS_DEFINES_H
#define H_EMULATOR_NETWORK_ACCESS_DEFINES_H

const unsigned int EMULATOR_NETWORK_ACCESS_STARTING_PORT = 65400;

typedef enum  {
    EMU_INFO,
    EMU_STATUS,
    EMU_PAUSE,
    EMU_STOP,
    EMU_RESET,
    EMU_RESUME,
    EMU_RELOAD,

    LOAD_GAME,
    GAME_INFO,

    CORES_LIST,
    CORE_INFO,
    CORE_CURRENT_INFO,
    LOAD_CORE,
    CORE_MEMORIES,
    CORE_READ,
    CORE_WRITE,

    DEBUG_BREAK,
    DEBUG_CONTINUE,

    LOAD_STATE,
    SAVE_STATE
} emulator_network_access_command;

#define EMU_NWA_COMMAND_STRING(cmd) {cmd, #cmd}

typedef struct {
    emulator_network_access_command command;
    const char* string;
} emulator_network_access_command_string_entry;

const emulator_network_access_command_string_entry emulator_network_access_command_strings[] = {
    EMU_NWA_COMMAND_STRING(EMU_INFO),
    EMU_NWA_COMMAND_STRING(EMU_STATUS),
    EMU_NWA_COMMAND_STRING(EMU_PAUSE),
    EMU_NWA_COMMAND_STRING(EMU_STOP),
    EMU_NWA_COMMAND_STRING(EMU_RESET),
    EMU_NWA_COMMAND_STRING(EMU_RESUME),
    EMU_NWA_COMMAND_STRING(EMU_RELOAD),
    EMU_NWA_COMMAND_STRING(LOAD_GAME),
    EMU_NWA_COMMAND_STRING(GAME_INFO),
    EMU_NWA_COMMAND_STRING(CORES_LIST),
    EMU_NWA_COMMAND_STRING(CORE_INFO),
    EMU_NWA_COMMAND_STRING(CORE_CURRENT_INFO),
    EMU_NWA_COMMAND_STRING(LOAD_CORE),
    EMU_NWA_COMMAND_STRING(CORE_MEMORIES),
    EMU_NWA_COMMAND_STRING(CORE_READ),
    EMU_NWA_COMMAND_STRING(CORE_WRITE),
    EMU_NWA_COMMAND_STRING(DEBUG_BREAK),
    EMU_NWA_COMMAND_STRING(DEBUG_CONTINUE),
    EMU_NWA_COMMAND_STRING(LOAD_STATE),
    EMU_NWA_COMMAND_STRING(SAVE_STATE)
};

const unsigned int emulator_network_access_number_of_command = sizeof(emulator_network_access_command_strings) / sizeof(emulator_network_access_command_strings[0]);

#undef EMU_NWA_COMMAND_STRING

#endif
