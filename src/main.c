#include "Hosebase/hosebase.h"

#include "networking.h"

inline void read_line(char* buffer, u32 buffer_size)
{
	memory_zero(buffer, buffer_size);
	fgets(buffer, buffer_size, stdin);
	u32 size = string_size(buffer);
	if (size) buffer[size - 1] = '\0';
}

inline b8 read_u32(u32* n)
{
	char str[40];
	read_line(str, 40);
	return string_to_u32(n, str);
}

#pragma pack(push)
#pragma pack(1)

typedef enum {
	HeaderType_ServerMessage,
	HeaderType_ClientMessage
} HeaderType;

typedef struct {
	HeaderType type;
} Header;

typedef struct {
	Header header;
	u32 client_id;
} ServerMessage;

typedef struct {
	Header header;
} ClientMessage;

#pragma pack(pop)

static void send_server_message(u32 client_id, const char* str)
{
	u8 new_msg[5000];
	memory_zero(new_msg, 5000);

	ServerMessage s;
	s.header.type = HeaderType_ServerMessage;
	s.client_id = client_id;

	u32 size = string_size(str);

	memory_copy(new_msg, &s, sizeof(s));
	memory_copy(new_msg + sizeof(s), str, size);

	web_server_send(&client_id, 1, TRUE, new_msg, sizeof(s) + size);
}

static void send_client_message(const char* str)
{
	u8 new_msg[5000];
	memory_zero(new_msg, 5000);

	ClientMessage s;
	s.header.type = HeaderType_ClientMessage;

	u32 size = string_size(str);

	memory_copy(new_msg, &s, sizeof(s));
	memory_copy(new_msg + sizeof(s), str, size);

	web_client_send(new_msg, sizeof(s) + size);
}

#ifdef SERVER

static void new_connection(u32 client_id, b8 connection)
{
	if (connection)
		SV_LOG_INFO("New Connection: %u\n", client_id);
	else
		SV_LOG_INFO("Client Disconnected: %u\n", client_id);
}

static void new_message(u32 client_id, const void* data, u32 size)
{
	const Header* header = data;

	switch (header->type)
	{

	case HeaderType_ClientMessage:
	{
		const char* str = (const char*)(header + 1);

		print("%u: %s\n", client_id, str);
		send_server_message(client_id, str);
	}
	break;

	}
}

int main()
{
	HosebaseInitializeDesc desc;
	memory_zero(&desc, sizeof(desc));
	desc.os.window.open = FALSE;
	desc.os.window.size = (v2_u32){ 1080u, 720u };

	if (!hosebase_initialize(&desc)) {
		return 1;
	}

	if (!net_initialize()) {
		return 1;
	}

	u32 port;
	
	while (1) {

		print("Port: ");
		b8 valid = read_u32(&port);

		if (!valid)
			print("Invalid port!\n");
		else break;
	}

	if (!web_server_initialize(port, 1000, new_connection, new_message)) {
		SV_LOG_ERROR("Can't start the server\n");
		return 1;
	}
	
	while (hosebase_frame_begin()) {

		char str[5000];
		read_line(str, 5000);

		if (string_equals(str, "exit")) {
			break;
		}
		if (string_equals(str, "help")) {
			print("TODO\n");
		}
		if (string_equals(str, "say")) {

			print("Say: ");
			read_line(str, 5000);
			send_server_message(u32_max, str);
		}
		else print("Invalid command\n");
		
		hosebase_frame_end();
	}

	web_server_close();

	net_close();

	hosebase_close();

	return 0;
}

#else

static void new_message(const void* data, u32 size)
{
	const Header* header = data;

	switch (header->type)
	{
	
	case HeaderType_ServerMessage:
	{
		const ServerMessage* msg = data;

		u32 client_id = msg->client_id;

		const char* str = (const char*)(msg + 1);
		if (client_id == u32_max)
			print("ADMIN: %s\n", str);
		else
			print("%u: %s\n", client_id, str);
	}
	break;

	}
	
}

static void disconnect(DisconnectReason reason)
{
	if (reason == DisconnectReason_ServerDisconnected)
		SV_LOG_INFO("Server Disconnected :(\n");
	else
		SV_LOG_INFO("WTF\n");
}

int main()
{
	HosebaseInitializeDesc desc;
	memory_zero(&desc, sizeof(desc));
	desc.os.window.open = FALSE;
	desc.os.window.size = (v2_u32){ 1080u, 720u };

	if (!hosebase_initialize(&desc)) {
		return 1;
	}

	if (!net_initialize()) {
		return 1;
	}

	char ip[40];
	print("IP: ");
	read_line(ip, 40);

	if (string_size(ip) == 0) {
		string_copy(ip, "192.168.216.235", 40);
	}

	u32 port;
	
	while (1) {

		print("Port: ");
		b8 valid = read_u32(&port);

		if (!valid)
			print("Invalid port!\n");
		else break;
	}

	if (!web_client_initialize(ip, port, 1000, new_message, disconnect)) {
		SV_LOG_ERROR("Can't start the client\n");
		return 1;
	}
	
	while (hosebase_frame_begin()) {

		char str[5000];
		read_line(str, 5000);

		if (string_equals(str, "exit")) {
			break;
		}
		if (string_equals(str, "help")) {
			print("TODO\n");
		}
		if (string_equals(str, "say")) {

			print("Say: ");
			read_line(str, 5000);
			send_client_message(str);
		}
		else print("Invalid command\n");

		hosebase_frame_end();
	}

	web_client_close();

	net_close();

	hosebase_close();

	return 0;
}

#endif
