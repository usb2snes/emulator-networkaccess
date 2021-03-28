
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include "dummy.h"
#include "emulator_network_access_defines.h"
#include "generic_poll_server.h"


#define start_hash_reply(S) generic_poll_server_start_hash_reply(S)
#define send_full_hash_reply(S, K, ...) generic_poll_server_send_full_hash_reply(S, K, __VA_ARGS__)
#define end_hash_reply(S) generic_poll_server_end_hash_reply(S)
#define send_hash_reply(S, K, ...) generic_poll_server_send_hash_reply(S, K, __VA_ARGS__)

const generic_emu_nwa_commands_map_t generic_emu_mwa_map = {
	{EMU_INFO, dummy_emu_info},
	{EMU_STATUS, dummy_emu_status},
	{EMU_PAUSE, dummy_emu_pause},
	{EMU_STOP, dummy_emu_stop},
	{EMU_RESET, dummy_emu_reset},
	{EMU_RESUME, dummy_emu_resume},
	{EMU_RELOAD, dummy_emu_reload},
	{LOAD_GAME, dummy_load_game},
	{GAME_INFO, dummy_game_info},
	{CORES_LIST, dummy_cores_list},
	{CORE_INFO, dummy_core_info},
	{CORE_CURRENT_INFO, dummy_core_current_info},
	{LOAD_CORE, dummy_core_load},
	{CORE_MEMORIES, dummy_core_memories},
	{CORE_READ, dummy_core_memory_read},
	{CORE_WRITE, dummy_core_memory_write},
	{DEBUG_BREAK, dummy_np},
	{DEBUG_CONTINUE, dummy_np},
	{LOAD_STATE, dummy_load_state},
	{SAVE_STATE, dummy_save_state}
};

const unsigned int generic_emu_mwa_map_size = 20;
void (*generic_poll_server_write_function)(SOCKET, char*, uint32_t) = &write_to_memory;


char*	hexString(const char* str, const unsigned int size)
{
    char* toret = malloc(size * 3 + 1);

    unsigned int i;
    for (i = 0; i < size; i++)
    {
        sprintf(toret + i * 3, "%02X ", (unsigned char) str[i]);
    }
    toret[size * 3] = 0;
    return toret;
}


#include "generic_poll_server.c"

enum status {
    RUNNING,
    PAUSED,
    STOPPED
};

enum status dummy_state = RUNNING;

char* sram_memory;
char* wram_memory;
char* rom_memory;


bool    snes9x_152_savestate_get_ram_wram(const char* savestate)
{
    char section_name[4];
    unsigned int    size = 0;

    section_name[3] = 0;
    printf("Opening savestate");
    FILE* fd = fopen(savestate, "rb");
    if (fd == NULL)
    {
        fprintf(stderr, "Can't open savesfile %s\n", savestate);
        exit(1);
    }
    fseek(fd, 14, SEEK_SET);
    while (fscanf(fd, "%3c:%06d:", &section_name, &size) == 2)
    {
        if (strcmp(section_name, "RAM") == 0)
        {
            wram_memory = (char*) malloc(size);
            char* buf = (char*) malloc(size);
            fread(buf, 1, size, fd);
            memcpy(wram_memory, buf, size);
            continue;
        }
        if (strcmp(section_name, "SRA") == 0)
        {
            sram_memory = (char*) malloc(size);
            char* buf = (char*) malloc(size);
            fread(buf, 1, size, fd);
            memcpy(sram_memory, buf, size);
            continue;
        }
        fseek(fd, size, SEEK_CUR);
    }
    return true;
}

void    prep_memory(const char* romfile, const char* savestate)
{
    struct stat file_stat;

    if (stat(romfile, &file_stat) < 0)
    {
        fprintf(stderr, "Can't stat the romfile %s\n", romfile);
        exit(1);
    }
    rom_memory = (char*) malloc(file_stat.st_size);
    FILE *file = fopen(romfile, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Can't open file %s\n", romfile);
        exit(1);
    }
    fread(rom_memory, 1, file_stat.st_size, file);
    snes9x_152_savestate_get_ram_wram(savestate);
}


int	main(int ac, char *ag[]) {
#ifdef _WIN32
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(1, 0), &wsadata) != 0)
	{
		fprintf(stderr, "Call to init Windows sockets failed. Do you have WinSock2 installed?");
		return false;
	}
#endif

    if (false && ac != 3)
    {
        fprintf(stderr, "dummyserver <romfile> <savestate>");
        exit(1);
    }
    prep_memory("F:\\Project\\QUsb2snes\\tests\\usb2snes-tests\\custom rom\\usb2snes-testlorom.sfc", "F:\\Tmp\\usb2snes-testlorom");
    printf("Starting Dummy Server\n");
	generic_poll_server_start();

}

static void write_to_socket(SOCKET socket, const char* str)
{
    write(socket, str, strlen(str));
}

bool dummy_emu_info(SOCKET socket, char ** args)
{
    write_to_socket(socket, "\nname:Dummy Emulator\nversion:0.1\n\n");
	return true;
}

bool dummy_emu_status(SOCKET socket, char ** args)
{
    write_to_socket(socket, "\n");
    if (dummy_state == RUNNING)
        write_to_socket(socket, "state:running");
    if (dummy_state == PAUSED)
        write_to_socket(socket, "state:paused");
    if (dummy_state == STOPPED)
        write_to_socket(socket, "state:stopped");
    write_to_socket(socket, "\ngame:dummy game\n\n");
	return true;
}

bool dummy_emu_pause(SOCKET socket, char ** args)
{
    dummy_state = PAUSED;
	return true;
}

bool dummy_emu_stop(SOCKET socket, char ** args)
{
    dummy_state = STOPPED;
	return true;
}

bool dummy_emu_reset(SOCKET socket, char ** args)
{
	return false;
}

bool dummy_emu_resume(SOCKET socket, char ** args)
{
    dummy_state = RUNNING;
	return true;
}

bool dummy_emu_reload(SOCKET socket, char ** args)
{
	return false;
}

bool dummy_load_game(SOCKET socket, char ** args)
{
	return false;
}

bool dummy_game_info(SOCKET socket, char ** args)
{
	return false;
}

bool dummy_cores_list(SOCKET socket, char ** args)
{
    send_full_hash_reply(socket, 3, "plateform", "SNES", 
                                    "name", "dummy core", 
                                    "version", "0.1"
    );
	return true;
}

bool dummy_core_info(SOCKET socket, char ** args)
{
    return dummy_cores_list(socket, args);
}

bool dummy_core_current_info(SOCKET socket, char ** args)
{
    return dummy_cores_list(socket, args);
}

bool dummy_core_load(SOCKET socket, char ** args)
{
	return false;
}

bool dummy_core_memories(SOCKET socket, char ** args)
{
    send_full_hash_reply(socket, 6, "name", "WRAM", "access", "rw",
                                    "name", "SRAM", "access", "rw",
                                    "name", "CARTROM", "access", "rw"
    );
    return true;
}

bool dummy_core_memory_read(SOCKET socket, char ** args)
{
    char* ptr = NULL;
    size_t offset = generic_poll_server_get_offset(args[1]);
    uint32_t size = atoi(args[2]);
    s_debug("Trying to read : %s - %06X:%d\n", args[0], offset, size);
    if (strcmp(args[0], "WRAM") == 0)
        ptr = wram_memory;
    if (strcmp(args[0], "SRAM") == 0)
        ptr = sram_memory;
    if (strcmp(args[0], "CARTROM") == 0)
        ptr = rom_memory;
    write(socket, "\0", 1);
    write(socket, &size, 4);
    s_debug("Sending : %s\n", hexString(ptr + offset, size));
    write(socket, ptr + offset, size);
	return true;
}

// This should probably be in a client specific struct

char*   memory_to_write = NULL;
size_t  offset_to_write = 0;
uint32_t size_written = 0;
uint32_t size_to_write = 0;

void    write_to_memory(SOCKET socket, char* data, uint32_t size)
{
    memcpy(memory_to_write + offset_to_write, data, size);
    size_written += size;
    if (size_written == size_to_write)
    {
        size_to_write = 0;
        size_written = 0;
        offset_to_write = 0;
        memory_to_write = NULL;
        write(socket, "\n\n", 2);
    }
}


// This does not handle all weird case sliver like

bool dummy_core_memory_write(SOCKET socket, char ** args)
{
	if (strcmp(args[0], "WRAM") == 0)
        memory_to_write = wram_memory;
    if (strcmp(args[0], "SRAM") == 0)
        memory_to_write = sram_memory;
    if (strcmp(args[0], "CARTROM") == 0)
        memory_to_write = rom_memory;
    offset_to_write = generic_poll_server_get_offset(args[1]);
    size_to_write = atoi(args[2]);
    size_written = 0;
    return true;
}

bool dummy_np(SOCKET socket, char ** args)
{
	return false;
}

bool dummy_load_state(SOCKET socket, char ** args)
{
	return false;
}

bool dummy_save_state(SOCKET socket, char ** args)
{
	return false;
}
