#include "networking.h"

#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

typedef struct {
	struct sockaddr_in hint;
} ClientRegister;

typedef struct {
	
	SOCKET socket;
	struct sockaddr_in hint;
	Thread thread;
	b8 running;
	u8* buffer;
	u32 buffer_capacity;

	ClientRegister* registred_clients;
	
} ServerData;

typedef struct {
	
	SOCKET socket;
	struct sockaddr_in server_hint;
	struct sockaddr_in hint;
	Thread thread;
	b8 running;
	u8* buffer;
	u32 buffer_capacity;
	
} ClientData;

typedef struct {
	
	WSADATA winsock_data;

	ServerData server;
	ClientData client;
	
} NetData;

#define HEADER_TYPE_CONNECT 0x00
#define HEADER_TYPE_CUSTOM 0x69
#define HEADER_TYPE_DISCONNECT 0xFF

typedef struct {
	u32 size;
	u8 type;
} NetHeader;

static NetData* net;

/////////////////////////////////////////////// SERVER /////////////////////////////////////////////////////////

inline void register_client_if_not_exists(struct sockaddr_in client)
{
	ServerData* s = &net->server;

	b8 found = FALSE;

	u32 size = array_size(&s->registred_clients);
	foreach(i, size) {

		if (s->registred_clients[i].hint.sin_addr.S_un.S_addr == client.sin_addr.S_un.S_addr) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		array_push(&s->registred_clients, client);

		char client_ip[256];
		memory_zero(client_ip, 256);
		inet_ntop(AF_INET, &client.sin_addr, client_ip, 256);
		
		SV_LOG_INFO("Client Connected: '%s'\n", client_ip);
	}
}

static u32 server_loop(void* arg)
{
	struct sockaddr_in client;
	i32 client_size = sizeof(client);
	memory_zero(&client, client_size);

	SOCKET socket = net->server.socket;
	u8* buffer = net->server.buffer;
	u32 buffer_capacity = net->server.buffer_capacity;

	while (net->server.running) {

		memory_zero(buffer, buffer_capacity);
		
		int bytes_in = recvfrom(socket, buffer, buffer_capacity, 0, (struct sockaddr*)&client, &client_size);

		if (bytes_in == SOCKET_ERROR) {

			SV_LOG_ERROR("Error reciving from client\n");
			continue;
		}

		u8* it = buffer;

		NetHeader* header = (NetHeader*)it;
		it += sizeof(NetHeader);

		switch (header->type) {

		case HEADER_TYPE_CONNECT:
			register_client_if_not_exists(client);
			break;

		case HEADER_TYPE_DISCONNECT:
			// TODO unregister_client_if_not_exists(client);
			break;

		case HEADER_TYPE_CUSTOM:
		{
			char client_ip[256];
			memory_zero(client_ip, 256);
			inet_ntop(AF_INET, &client.sin_addr, client_ip, 256);
			
			SV_LOG_INFO("'%s': %u bytes\n", client_ip, header->size);
		}
		break;
			
		}

		// thread_yeld();
	}

	closesocket(net->server.socket);

	return 0;
}

b8 web_server_initialize(u32 port, u32 buffer_capacity)
{
	ServerData* s = &net->server;
	
	s->socket = socket(AF_INET, SOCK_DGRAM, 0);

	SV_ZERO(s->hint);
				
	s->hint.sin_addr.S_un.S_addr = ADDR_ANY;
	s->hint.sin_family = AF_INET;
	s->hint.sin_port = htons(port); // From little to big endian

	if (bind(s->socket, (struct sockaddr*)&s->hint, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {

		SV_LOG_ERROR("Can't connect server socket\n");
		return FALSE;
	}

	s->running = TRUE;
	s->buffer_capacity = SV_MAX(buffer_capacity, 1000);
	s->buffer = memory_allocate(s->buffer_capacity);

	// TEMP
	s->registred_clients = array_init(ClientRegister, 10, 1.5f);

	s->thread = thread_create(server_loop, NULL);

	return s->thread != NULL;
}

void web_server_close()
{
	ServerData* s = &net->server;
	
	s->running = FALSE;
	thread_wait(s->thread);
}

/////////////////////////////////////////////// CLIENT /////////////////////////////////////////////////////////

static u32 client_loop(void* arg)
{
	ClientData* d = &net->client;

	u8* buffer = d->buffer;
	u32 buffer_capacity = d->buffer_capacity;
	SOCKET socket = d->socket;
	
	while (d->running) {

		memory_zero(buffer, buffer_capacity);
		
		int bytes_in = recvfrom(socket, buffer, buffer_capacity, 0, NULL, NULL);

		if (bytes_in == SOCKET_ERROR) {

			SV_LOG_ERROR("Error reciving from server\n");
			continue;
		}

		SV_LOG_INFO("Message: %s\n", buffer);

		// thread_yeld();
	}

	return 0;
}

b8 _client_send(const void* data, u32 size)
{
	int ok = sendto(net->client.socket, data, size, 0, (struct sockaddr*)&net->client.server_hint, sizeof(net->client.server_hint));

	if (ok == SOCKET_ERROR) {
		SV_LOG_ERROR("Can't send data to the server\n");
		return FALSE;
	}

	return TRUE;
}

b8 web_client_initialize(const char* ip, u32 port, u32 buffer_capacity)
{
	ClientData* d = &net->client;

	// Fill server hint
	{
		struct sockaddr_in server;
		SV_ZERO(server);
	
		server.sin_port = htons(port); // From little to big endian
		server.sin_family = AF_INET;
		inet_pton(AF_INET, ip, &server.sin_addr);

		d->server_hint = server;
	}

	// Init some data
	{
		d->running = TRUE;
		d->buffer_capacity = SV_MAX(buffer_capacity, 1000);
		d->buffer = memory_allocate(d->buffer_capacity);
		d->socket = socket(AF_INET, SOCK_DGRAM, 0);
	}

	// Compute client ID
	/*{
		u64 id = (u64)(timer_now() * 1000000000.0);
		id = hash_combine(0x324F324, id);

		d->id = id;
		}*/

	// Fill client hint
	{
		SV_ZERO(d->hint);
		d->hint.sin_addr.S_un.S_addr = ADDR_ANY;
		d->hint.sin_family = AF_INET;
		d->hint.sin_port = htons(port); // From little to big endian
	}

	b8 recived = FALSE;

	// Bind
	/*{
		i32 res = bind(d->socket, (struct sockaddr*)&d->server_hint, sizeof(struct sockaddr_in));

		if (res == SOCKET_ERROR) {
			
			SV_LOG_ERROR("Can't connect client socket\n");
			return FALSE;
		}
		}*/

	// Send connection message
	{
		NetHeader msg;
		msg.type = HEADER_TYPE_CONNECT;
		msg.size = 0;

		_client_send(&msg, sizeof(msg));
	}

	// Start thread
	d->thread = thread_create(client_loop, NULL);
	
	// TODO: Handle errors
	return TRUE;
}

void web_client_close()
{
	closesocket(net->client.socket);
}

b8 web_client_send(const void* data, u32 size)
{
	NetHeader header;
	header.type = HEADER_TYPE_CUSTOM;
	header.size = size;

	// TODO
	u8 b[1000];
	memory_zero(b, 1000);
	memory_copy(b, &header, sizeof(header));
	
	memory_copy(b + sizeof(header), data, size);

	return _client_send(b, sizeof(header) + size);
}

b8 net_initialize()
{
	net = memory_allocate(sizeof(NetData));
	
	i32 res = WSAStartup(MAKEWORD(2, 2), &net->winsock_data);

	if (res != 0) {

		SV_LOG_ERROR("Can't start WinSock\n");
		return FALSE;
	}

	return TRUE;
}

void net_close()
{
	if (net) {
		
		WSACleanup();

		memory_free(net);
	}
}
