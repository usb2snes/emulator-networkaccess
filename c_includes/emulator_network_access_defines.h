#ifndef H_EMULATOR_NETWORK_ACCESS_DEFINES_H
#define H_EMULATOR_NETWORK_ACCESS_DEFINES_H

const unsigned int EMULATOR_NETWORK_ACCESS_STARTING_PORT = 65400;

typedef enum  {
    EMULATOR_INFO,
    EMULATION_STATUS,
    EMULATION_PAUSE,
    EMULATION_STOP,
    EMULATION_RESET,
    EMULATION_RESUME,
    EMULATION_RELOAD,

    MY_NAME_IS,

    LOAD_GAME,
    GAME_INFO,

    CORES_LIST,
    CORE_INFO,
    CORE_CURRENT_INFO,
    LOAD_CORE,
    CORE_MEMORIES,
    CORE_READ,
    bCORE_WRITE,

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
    EMU_NWA_COMMAND_STRING(EMULATOR_INFO),
    EMU_NWA_COMMAND_STRING(EMULATION_STATUS),
    EMU_NWA_COMMAND_STRING(EMULATION_PAUSE),
    EMU_NWA_COMMAND_STRING(EMULATION_STOP),
    EMU_NWA_COMMAND_STRING(EMULATION_RESET),
    EMU_NWA_COMMAND_STRING(EMULATION_RESUME),
    EMU_NWA_COMMAND_STRING(EMULATION_RELOAD),
    EMU_NWA_COMMAND_STRING(MY_NAME_IS),
    EMU_NWA_COMMAND_STRING(LOAD_GAME),
    EMU_NWA_COMMAND_STRING(GAME_INFO),
    EMU_NWA_COMMAND_STRING(CORES_LIST),
    EMU_NWA_COMMAND_STRING(CORE_INFO),
    EMU_NWA_COMMAND_STRING(CORE_CURRENT_INFO),
    EMU_NWA_COMMAND_STRING(LOAD_CORE),
    EMU_NWA_COMMAND_STRING(CORE_MEMORIES),
    EMU_NWA_COMMAND_STRING(CORE_READ),
    EMU_NWA_COMMAND_STRING(bCORE_WRITE),
    EMU_NWA_COMMAND_STRING(DEBUG_BREAK),
    EMU_NWA_COMMAND_STRING(DEBUG_CONTINUE),
    EMU_NWA_COMMAND_STRING(LOAD_STATE),
    EMU_NWA_COMMAND_STRING(SAVE_STATE)
};

const unsigned int emulator_network_access_number_of_command = sizeof(emulator_network_access_command_strings) / sizeof(emulator_network_access_command_strings[0]);

#undef EMU_NWA_COMMAND_STRING

typedef enum {
    protocol_error,
    invalid_command,
    invalid_argument,
    command_error,
    not_allowed
} emulator_network_access_error_type;

typedef struct {
    emulator_network_access_error_type error_type;
    const char* string;
} emulator_network_access_error_type_string_entry;

#define EMU_NWA_ERROR_STRING(cmd) {cmd, #cmd}

const emulator_network_access_error_type_string_entry emulator_network_access_error_type_strings[] = {
    EMU_NWA_ERROR_STRING(protocol_error),
    EMU_NWA_ERROR_STRING(invalid_command),
    EMU_NWA_ERROR_STRING(invalid_argument),
    EMU_NWA_ERROR_STRING(command_error),
    EMU_NWA_ERROR_STRING(not_allowed)
};

#undef EMU_NWA_ERROR_STRING
#endif
