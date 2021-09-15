#pragma once

#include "Hosebase/hosebase.h"

// WEB

typedef void(*WebServerConnectionFn)(u32 client_id, b8 connection);
typedef void(*WebServerMessageFn)(u32 client_id, const void* data, u32 size);

b8   web_server_initialize(u32 port, u32 buffer_capacity, WebServerConnectionFn connection_fn, WebServerMessageFn message_fn);
void web_server_close();
b8   web_server_send(u32* clients, u32 client_count, const void* data, u32 size);

typedef enum {
	DisconnectReason_Unknown,
	DisconnectReason_ServerDisconnected,
	DisconnectReason_Timeout,
} DisconnectReason;

typedef void(*WebClientMessageFn)(const void* data, u32 size);
typedef void(*WebClientDisconnectFn)(DisconnectReason reason);

b8   web_client_initialize(const char* ip, u32 port, u32 buffer_capacity, WebClientMessageFn message_fn, WebClientDisconnectFn disconnect_fn);
void web_client_close();
b8   web_client_send(const void* data, u32 size);

b8 net_initialize();
void net_close();
