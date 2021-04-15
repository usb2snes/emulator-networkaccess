
#include <stdbool.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MY_DEBUG 1
#ifdef MY_DEBUG
#define s_debug(...)  printf(__VA_ARGS__)
#else
#define s_debug(...)
#endif

#include <stdarg.h>

#ifdef _WIN32
#include <winsock2.h>
#include <process.h>
#define close(h) if(h){closesocket(h);}
#define write(a,b,c) send(a, b, c, 0)
#define poll(a, b, c) WSAPoll(a, b, c);
typedef LPWSAPOLLFD poll_fd_t;
#else
#include <poll.h>
	typedef int SOCKET;
	typedef struct pollfd poll_fd_t;

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#define ioctlsocket ioctl
#endif

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifdef _WIN32

/* On windows, recv return an error when a non blocking socket
*  has nothing left to read. This is wrapper to return 0 instead.
*/
static inline int	read(SOCKET s, char* b, int size)
{
	int res = recv(s, b, size, 0);
	if (res == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return 0;
		return -1;
	}
	return res;
}
#endif


#include "generic_poll_server.h"

static void __send_hash_reply(SOCKET socket, bool end, int key_count, va_list list)
{
    for (unsigned int i = 0; i < key_count; i++)
    {
        char* key = va_arg(list, char*);
        char* value = va_arg(list, char*);
		unsigned int size_kv = strlen(key) + strlen(value);
        char* buffer = (char*) malloc(size_kv + 3);
        snprintf(buffer, size_kv + 2, "%s:%s", key, value);
		buffer[size_kv + 1] = '\n';
        write(socket, buffer, size_kv + 2);
		buffer[size_kv + 2] = 0;
		s_debug("Sending : %s", buffer);
		free(buffer);
        //dprintf(socket, "%s=%s\n", key, value); This does not exists on windows
    }
	if (end)
		generic_poll_server_end_hash_reply(socket);
}

inline void    generic_poll_server_start_hash_reply(SOCKET socket)
{
    write(socket, "\n", 1);
}

inline void    generic_poll_server_end_hash_reply(SOCKET socket)
{
    write(socket, "\n", 1);
}

void    generic_poll_server_send_hash_reply(SOCKET socket, int key_count, ...)
{
    va_list list;
    va_start(list, key_count);

    __send_hash_reply(socket, false, key_count, list);
    va_end(list);
}

void    generic_poll_server_send_full_hash_reply(SOCKET socket, int key_count, ...)
{
    va_list list;
    va_start(list, key_count);
    generic_poll_server_start_hash_reply(socket);
    __send_hash_reply(socket, true, key_count, list);
    va_end(list);
}

size_t          generic_poll_server_get_offset(const char *offset_str)
{
    if (offset_str[0] == '$')
    {
        size_t offset = 0;
        sscanf(offset_str + 1, "%zX", &offset);
        return offset;
    }
    return atoll(offset_str);
}

static generic_poll_server_client clients[5];


static generic_poll_server_client* get_client(SOCKET client_fd)
{
	for (unsigned int i = 0; i < 5; i++)
	{
		if (clients[i].socket_fd == client_fd)
			return &clients[i];
	}
	return NULL;
}

static void remove_client(SOCKET client_fd)
{
	for (unsigned int i = 0; i < 5; i++)
	{
		if (clients[i].socket_fd == client_fd)
		{
			clients[i].socket_fd = 0;
			return;
		}
	}
}

static bool	add_to_clients(SOCKET client_fd)
{
	for (unsigned int i = 0; i < 5; i++)
	{
		if (clients[i].socket_fd == 0)
		{
			s_debug("Adding new client %d\n", client_fd);
			clients[i].socket_fd = client_fd;
			clients[i].pending_data[0] = 0;
			clients[i].pending_size = 0;
            clients[i].pending_pos = 0;
			clients[i].in_cmd = false;
			clients[i].write_handled_size = 0;
			clients[i].write_expected_size = 0;
			
			return true;
		}
	}
	return false;
}

static void send_error(SOCKET fd, const char* error_string)
{
    char* tosend = (char*) malloc(strlen(error_string) + strlen("\nerror:\n\n") + 1);
    int towrite = snprintf(tosend, strlen(error_string) + strlen("\nerror:\n\n") + 1,
                           "\nerror:%s\n\n", 
                           error_string);
    write(fd, tosend, towrite);
}

static void execute_command(generic_poll_server_client* client, char *cmd_str, char **args, int args_count)
{
    emulator_network_access_command command;
    bool    valid_command = false;
    for (unsigned int i = 0; i < emulator_network_access_number_of_command; i++)
    {
        if (strcmp(cmd_str, emulator_network_access_command_strings[i].string) == 0)
        {
            command = emulator_network_access_command_strings[i].command;
            valid_command = true;
        }
    }
    if (!valid_command)
    {
        send_error(client->socket_fd, "Unvalid command");
        return ;
    }
    client->current_command = command;
    for (unsigned int i = 0; i < generic_emu_mwa_map_size; i++)
    {
        if ( generic_emu_mwa_map[i].command == command)
        {
            bool success = generic_emu_mwa_map[i].function(client->socket_fd, args, args_count);
            if (!success)
            {
                send_error(client->socket_fd, "The command did not success");
            }
        }
    }
}

static void process_command(generic_poll_server_client* client)
{
	if (client->in_cmd)
	{
		if (client->current_command == CORE_WRITE)
		{
            bool result = generic_poll_server_write_function(client->socket_fd, client->pending_data, client->pending_size);
            if (result)
                client->in_cmd = false;
			// do stuff to core write
		}
		return;
	}
	char **args = (char **) malloc(10 * sizeof(char*));
	char cmd[50];
	bool cmd_has_arg = true;

    args[0] = NULL;
	char* cmd_block = client->pending_data;
    client->in_cmd = true;
    s_debug("Command block is : %s\n", cmd_block);

	for (unsigned int i = 0; i < client->pending_size; i++)
	{
		if (cmd_block[i] == ' ' || cmd_block[i] == '\n')
		{
			memcpy(cmd, cmd_block, i);
            s_debug("i : %d\n", i);
			cmd[i] = 0;
			if (cmd_block[i] == '\n')
				cmd_has_arg = false;
			break;
		}
	}
	s_debug("Cmd : %s\n", cmd);
	cmd_block = cmd_block + strlen(cmd);
    unsigned int nb_arg = 0;
	if (cmd_has_arg)
	{
		unsigned int i = 0;
		unsigned int arg_idx = 0;
		unsigned int arg_i = 0;
        nb_arg = 1;
        args[0] = (char*) malloc(2056);
		while (cmd_block[i] == ' ')
			i++;
		while (cmd_block[i] != '\n')
		{
			if (cmd_block[i] == ';')
			{
				args[arg_idx][arg_i] = 0;
				arg_idx++;
                nb_arg++;
                args[arg_idx] = (char*) malloc(2056);
				arg_i = 0;
                i++;
                continue;
			}
			args[arg_idx][arg_i] = cmd_block[i];
			arg_i++;
			i++;
		}
		args[arg_idx][arg_i] = 0;
	}
	s_debug("Executing command : %s - %d args\n", cmd, nb_arg);
	execute_command(client, cmd, args, nb_arg);
    client->in_cmd = false;
    if (client->current_command == CORE_WRITE)
	{
        s_debug("Core write command\n");
        client->write_handled_size = 0;
        // No size provided, fuuuu, thanks Sliver !
		if (nb_arg <= 2)
        {
            client->write_expected_size = 0;
            
        } else {
            unsigned int total_size = 0;
            for (unsigned int i = 2; i < nb_arg; i += 2)
            {
		        total_size += atoi(args[i]);
			}
            client->write_expected_size = total_size + 4;
        }
        client->in_cmd = true;
	}
    for (unsigned int i = 0; i < nb_arg; i++)
    {
        free(args[i]);
    }
    free(args);
}


static void	preprocess_data(generic_poll_server_client* client)
{
	unsigned int read_pos = 0;
	printf("Pre:Read size : %d\n", client->readed_size);
    s_debug("Data : %s\n", hexString(client->readed_data, client->readed_size));
	while (read_pos == 0 || read_pos < client->readed_size)
	{
        s_debug("Client in command : %d\n", client->in_cmd);
		if (client->in_cmd)
		{
			s_debug("Pre:WRITE\n");
            s_debug("Data : %s\n", hexString(client->readed_data, client->readed_size));
			unsigned int to_complete_size = 0;
			if (client->current_command == CORE_WRITE)
            {
                if (client->write_handled_size < 4)
                {
                    if (client->write_handled_size == 0 && *(client->readed_data + read_pos) != 0)
                    {
                        send_error(client->socket_fd, "Expected binary data");
                        //FIXME reinit state and close the connection?
                    }
                    unsigned int to_copy_s = MIN(5 - client->write_handled_size, client->readed_size - read_pos);
                    if (client->write_handled_size == 0) // we don't want the byte that tell us it's binary.
                    {
                        to_copy_s -= 1;
                        read_pos++;
                    }
                    memcpy(client->pending_data + client->write_handled_size, client->readed_data + read_pos, to_copy_s);
                    client->pending_size += to_copy_s;
                    client->write_handled_size += to_copy_s;
                    read_pos += to_copy_s;
                    client->pending_pos += to_copy_s;
                }
                
                if (client->write_expected_size == 0)
                {
                    if (client->write_handled_size == 4)
                    {
                        s_debug("Data : %s\n", hexString(client->pending_data, 4));
                        client->write_expected_size = ntohl(*((uint32_t*)(client->pending_data)));
                        s_debug("Expected size from binary header %d\n", client->write_expected_size);
                    }
                }
				to_complete_size = client->write_expected_size - client->write_handled_size + 4;
                s_debug("Expected size : %d - to complete :%d\n", client->write_expected_size, to_complete_size);
            }
            if (client->write_handled_size >= 4)
            {
			    // Read data contains etheir all data or partial.
    			if (to_complete_size <= client->readed_size - read_pos)
	    		{
				    printf("get all write data\n");
				    memcpy(client->pending_data + client->pending_pos, client->readed_data + read_pos, to_complete_size);
				    client->pending_size += to_complete_size;
                    client->write_handled_size += to_complete_size;
			    	read_pos += to_complete_size;
		    	}
	    		else {
    				printf("Partial write data\n");
			    	memcpy(client->pending_data + client->pending_pos, client->readed_data + read_pos, client->readed_size - read_pos);
		    		client->pending_size += client->readed_size - read_pos;
                    client->write_handled_size += client->readed_size - read_pos;
    				read_pos = client->readed_size;
			    }
			    process_command(client);
			    client->pending_pos = 0;
			    client->pending_size = 0;
            }
			continue;
		}
		s_debug("Pre:Checking \\n\n");
		bool has_endl = false;
		for (unsigned int i = read_pos; i < client->readed_size; i++)
		{
			if (client->readed_data[i] == '\n')
			{
				has_endl = true;
				memcpy(client->pending_data + client->pending_pos, client->readed_data + read_pos, i + 1 - read_pos);
				client->pending_size += i + 1 - read_pos;
				read_pos = i + 1;
				process_command(client);
				client->pending_pos = 0;
				client->pending_size = 0;
				printf("read pos: %d\n", read_pos);
				break;
			}
		}
		if (has_endl)
			continue;
		// we should have reached unprocessed data
		memcpy(client->pending_data + client->pending_pos, client->readed_data + read_pos, client->readed_size - read_pos);
		client->pending_pos += client->readed_size - read_pos;
		client->pending_size += client->readed_size - read_pos;
		printf("ppos, ppsize : %d, %d\n", client->pending_pos, client->pending_size);
		break;
	}
}

// CORE;_MEMOR;Y_WRITE blalblalb\ndata;
// COREMWRITE blablalba\ndata;


static bool read_client_data(SOCKET fd)
{
	char    buff[2048];

	int read_size;
	generic_poll_server_client* client = get_client(fd);
	read_size = read(fd, &buff, 2048);
	s_debug("Readed %d data on %d\n", read_size, fd);
	if (read_size <= 0)
		return false;
	client->readed_size = read_size;
	memcpy(client->readed_data, &buff, read_size);
	preprocess_data(client);
	return true;
}

long long milliseconds_now() {
#if defined WIN32 || defined _WIN32
    static LARGE_INTEGER s_frequency;
    static BOOL s_use_qpc = TRUE;
	QueryPerformanceFrequency(&s_frequency);
    if (s_use_qpc) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (1000LL * now.QuadPart) / s_frequency.QuadPart;
    } else {
        return GetTickCount();
    }
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)(ts.tv_nsec / 1000000) + ((long long)ts.tv_sec * 1000ll);
#endif
}


static bool generic_poll_server_start()
{
	SOCKET server_socket;
	struct sockaddr_in name;

	for (unsigned int i = 0; i < 5; i++)
		clients[i].socket_fd = 0;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		fprintf(stderr, "Error while creating socket : %s\n", strerror(errno));
		return 1;
	}
	name.sin_family = AF_INET;
	name.sin_port = htons(EMULATOR_NETWORK_ACCESS_STARTING_PORT);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	unsigned int cpt = 0;
	int bind_return = bind(server_socket, (struct sockaddr*) &name, sizeof(name));
	while (bind_return < 0 && cpt != 4)
	{
		cpt++;
		printf("Can't bind the socket on port %s, %d, trying next port\n", strerror(errno), EMULATOR_NETWORK_ACCESS_STARTING_PORT + cpt);
		name.sin_port = htons(EMULATOR_NETWORK_ACCESS_STARTING_PORT + cpt);
		bind_return = bind(server_socket, (struct sockaddr*) &name, sizeof(name));
	}
	if (bind_return < 0)
	{
		fprintf(stderr, "Error while binding socket : %s\n", strerror(errno));
		return false;
	}
	if (listen(server_socket, 1) < 0)
	{
		fprintf(stderr, "Error while listenning : %s\n", strerror(errno));
		return false;
	}
	printf("Dummy server ready; listening on %d\n", EMULATOR_NETWORK_ACCESS_STARTING_PORT + cpt);
	long long now = milliseconds_now();
	s_debug("Time : %lld\n", milliseconds_now());
	struct pollfd	poll_fds[6];
	unsigned int	poll_fds_count = 1;
	poll_fds[0].fd = server_socket;
	poll_fds[0].events = POLLIN;
	while (true)
	{
		s_debug("Waiting for poll\n");
		int ret = poll(poll_fds, poll_fds_count, -1);
		s_debug("%lld - Poll returned : %d\n", milliseconds_now() - now, ret);
		// New clients ?
		if (poll_fds[0].revents & POLLIN)
		{
			printf("New client connection\n");
			SOCKET new_socket;
			int size = sizeof(struct sockaddr_in);
			struct sockaddr_in new_client;
			new_socket = accept(server_socket, (struct sockaddr *) &new_client, &size);
			if (new_socket < 0)
			{
				fprintf(stderr, "Error while listenning : %s\n", strerror(errno));
				return 1;
			}
			if (add_to_clients(new_socket))
			{
				int piko = 1;
				ioctlsocket(new_socket, FIONBIO, &piko);
				s_debug("Server: connection from host %s, port %hd.\n",
					inet_ntoa(new_client.sin_addr),
					ntohs(new_client.sin_port));
				poll_fds[poll_fds_count].fd = new_socket;
				poll_fds[poll_fds_count].events = POLLIN;
				poll_fds_count++;
			} else {
				fprintf(stderr, "execing max client number : %d\n", 5);
				close(new_socket);
			}
		}

		for (unsigned int i = 1; i < poll_fds_count; i++)
		{
			if (poll_fds[i].revents & POLLIN)
			{
                s_debug("New data on client : %d\n", i);
				if (read_client_data(poll_fds[i].fd) == false)
                {
                    s_debug("Disconnecting client\n");
                    remove_client(poll_fds[i].fd);
                    if (poll_fds[i].fd != poll_fds[poll_fds_count - 1].fd)
                    {
                        // We are not removing the last one
                        poll_fds[i] = poll_fds[poll_fds_count - 1];
                        i--;
                    }
                    poll_fds_count--;
                }
			}
			// This is when the socket is not closed nicely
			if (poll_fds[i].revents & POLLERR)
			{
				s_debug("Disconnecting client\n");
				remove_client(poll_fds[i].fd);
				if (poll_fds[i].fd != poll_fds[poll_fds_count - 1].fd)
				{
					// We are not removing the last one
					poll_fds[i] = poll_fds[poll_fds_count - 1];
					i--;
				}
				poll_fds_count--;
			}
		}
	}
}
