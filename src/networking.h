#pragma once

#include "Hosebase/hosebase.h"

// WEB

b8   web_server_initialize(u32 port, u32 buffer_capacity);
void web_server_close();

b8   web_client_initialize(const char* ip, u32 port, u32 buffer_capacity);
void web_client_close();
b8   web_client_send(const void* data, u32 size);

b8 net_initialize();
void net_close();
