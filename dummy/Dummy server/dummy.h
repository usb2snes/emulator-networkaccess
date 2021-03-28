#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#else
typedef int SOCKET;
#endif


bool	dummy_emu_info(SOCKET socket, char **args);
bool	dummy_emu_status(SOCKET socket, char **args);
bool	dummy_emu_pause(SOCKET socket, char **args);
bool	dummy_emu_stop(SOCKET socket, char **args);
bool	dummy_emu_reset(SOCKET socket, char **args);
bool	dummy_emu_resume(SOCKET socket, char **args);
bool	dummy_emu_reload(SOCKET socket, char **args);
bool	dummy_load_game(SOCKET socket, char **args);
bool	dummy_game_info(SOCKET socket, char **args);
bool	dummy_cores_list(SOCKET socket, char **args);
bool	dummy_core_info(SOCKET socket, char **args);
bool	dummy_core_current_info(SOCKET socket, char **args);
bool	dummy_core_load(SOCKET socket, char **args);
bool	dummy_core_memories(SOCKET socket, char **args);
bool	dummy_core_memory_read(SOCKET socket, char **args);
bool	dummy_core_memory_write(SOCKET socket, char **args);
bool	dummy_np(SOCKET socket, char **args);
bool	dummy_load_state(SOCKET socket, char **args);
bool	dummy_save_state(SOCKET socket, char **args);
	
void    write_to_memory(SOCKET socket, char* data, uint32_t size);
