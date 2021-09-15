#include "Hosebase/hosebase.h"

#include "networking.h"

#ifdef SERVER

int main()
{
	HosebaseInitializeDesc desc;
	memory_zero(&desc, sizeof(desc));
	desc.os.window.open = FALSE;

	if (!hosebase_initialize(&desc)) {
		return 1;
	}

	if (!net_initialize()) {
		return 1;
	}

	if (!web_server_initialize(54000, 1000)) {
		SV_LOG_ERROR("Can't start the server\n");
		return 1;
	}
	
	while (hosebase_frame_begin()) {
		hosebase_frame_end();
	}

	// TODO: web_server_close();

	net_close();

	hosebase_close();

	return 0;
}

#else

int main()
{
	HosebaseInitializeDesc desc;
	memory_zero(&desc, sizeof(desc));
	desc.os.window.open = TRUE;
	desc.os.window.size = (v2_u32){ 1080u, 720u };

	if (!hosebase_initialize(&desc)) {
		return 1;
	}

	if (!net_initialize()) {
		return 1;
	}

	if (!web_client_initialize("192.168.1.104", 54000, 1000)) {
		SV_LOG_ERROR("Can't start the client\n");
		return 1;
	}
	
	while (hosebase_frame_begin()) {

		if (input_key(Key_A, InputState_Released)) {

			const char* buffer = "Miaaau :)";
			web_client_send(buffer, string_size(buffer) + 1);
		}

		hosebase_frame_end();
	}

	web_client_close();

	net_close();

	hosebase_close();

	return 0;
}

#endif
