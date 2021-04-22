#include "lua_loader.h"

#include <Windows.h>

class c_lua_interface
{
public:
    void run_string(char const* filename, char const* path, char const* string_toun, bool run = true, bool show_errors = true)
    {
        using fn = void(__thiscall*)(c_lua_interface*, char const*, char const*, char const*, bool, bool);
        return (*(fn**)this)[92](this, filename, path, string_toun, run, show_errors);
    }
};
class c_lua_shared
{
public:
    c_lua_interface* get_interface(unsigned char state)
    {
        using fn = c_lua_interface * (__thiscall*)(c_lua_shared*, unsigned char);
        return (*(fn**)this)[6](this, state);
    }
};

typedef void* (*abstract_interface)(const char* name, int* return_code);

c_lua_shared* get_lua_shared()
{
    auto create_interface = (abstract_interface)GetProcAddress(GetModuleHandleA("lua_shared.dll"), "CreateInterface");
    return (c_lua_shared*)create_interface("LUASHARED003", 0);
}

class engine_client
{
public:
    auto is_ingame() -> bool
    {
        using fn = bool(__thiscall*)(void*);
        return (*(fn**)this)[26](this);
    }
};

engine_client* get_engine_client()
{
    auto create_interface = (abstract_interface)GetProcAddress(GetModuleHandleA("engine.dll"), "CreateInterface");
    return (engine_client*)create_interface("VEngineClient015", 0);
}

c_lua_shared* lua_shared = nullptr;
engine_client* engine = nullptr;

void lua_loader::init_lua_loader()
{
    lua_shared = get_lua_shared();
    engine = get_engine_client();
}

void lua_loader::load_lua(const std::string& lua)
{
    if (!engine->is_ingame())
        return;
    auto* current_interface = lua_shared->get_interface(0);
    if (current_interface)
        current_interface->run_string("sandbox.lua", "FUCK/YOU", lua.c_str());
}