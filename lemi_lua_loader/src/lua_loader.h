#pragma once
#include <string>

namespace lua_loader
{
	void init_lua_loader();
	void load_lua(const std::string& lua);
}
