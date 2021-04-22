#include <Windows.h>
#include <thread>

#include "hooks.h"
#include "lua_loader.h"

HMODULE dll_instance = nullptr;

void start()
{
	lua_loader::init_lua_loader();
	hooks::init_hooks();
}


BOOL WINAPI DllMain(HMODULE dll, DWORD  reason_for_call, LPVOID reserved)
{
	dll_instance = dll;
	DisableThreadLibraryCalls(dll);
	if (reason_for_call == DLL_PROCESS_ATTACH)
		std::thread(start).detach(); //CreateThread suck
	
	return TRUE;
}
