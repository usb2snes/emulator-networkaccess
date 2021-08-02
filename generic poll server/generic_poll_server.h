#ifndef H_GENERIC_POLL_SERVER_H
#define H_GENERIC_POLL_SERVER_H

#include <stdbool.h>
#include "emulator_network_access_defines.h"

typedef struct
{
    emulator_network_access_command command;
    bool(*function)(SOCKET, char**, int);
} generic_emu_nwa_command_entry;

typedef generic_emu_nwa_command_entry generic_emu_nwa_commands_map_t[];

#ifdef _WIN32
#include <winsock2.h>
#else
typedef int SOCKET;
#endif

typedef enum
{
    NEW_CLIENT,
    REMOVED_CLIENT,
    SERVER_STARTED
}  generic_poll_server_callback;

typedef struct {
    bool(*add_client)(SOCKET socket);
    bool(*remove_client)(SOCKET socket);
    bool(*server_started)(int port);
} generic_poll_server_callbacks;

typedef struct {
    SOCKET  socket_fd;
    char    readed_data[2048];
    char    pending_data[2048];
    int     readed_size;
    int     pending_size;
    int     pending_pos;

    bool    in_cmd;

    emulator_network_access_command         current_command;

    char                write_binary_header[5];
    char                write_binary_header_size;
    unsigned int        write_expected_size;
    unsigned int        write_handled_size;


} generic_poll_server_client;



void            generic_poll_server_add_callback(generic_poll_server_callback cb, void* fntptr);

/*
    use it like that :
    send_hash_reply(socket, "key", "value", "key2", "value2")
*/

void            generic_poll_server_send_hash_reply(SOCKET socket, int key_count, ...);
void            generic_poll_server_send_full_hash_reply(SOCKET socket, int key_count, ...);
void            generic_poll_server_end_hash_reply(SOCKET socket);
void            generic_poll_server_start_hash_reply(SOCKET socket);
size_t          generic_poll_server_get_offset(const char *offset_str);


#endif
