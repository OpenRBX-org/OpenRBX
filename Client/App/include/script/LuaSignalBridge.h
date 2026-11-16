#ifndef SCRIPT_LUASIGNALBRIDGE_H
#define SCRIPT_LUASIGNALBRIDGE_H

#include "lua/LuaBridge.h"
#include "reflection/signal.h"
#include "util/Signals.h"

namespace RBX {
namespace Lua {

template <>
int Bridge<Signals::Connection, 1>::on_index(const Signals::Connection& value, const char* name, lua_State* L);

template <>
void Bridge<Signals::Connection, 1>::on_newindex(Signals::Connection& value, const char* name, lua_State* L);

template <>
inline int Bridge<Signals::Connection, 1>::on_tostring(const Signals::Connection& value, lua_State* L)
{
	lua_pushstring(L, "Connection");
	return 1;
}

template <>
int Bridge<std::shared_ptr<RBX::Reflection::SignalInstance>, 0>::on_index(
	const std::shared_ptr<RBX::Reflection::SignalInstance>& value, const char* name, lua_State* L
);

template <>
void Bridge<std::shared_ptr<RBX::Reflection::SignalInstance>, 0>::on_newindex(
	std::shared_ptr<RBX::Reflection::SignalInstance>& value, const char* name, lua_State* L
);

template <>
inline int Bridge<std::shared_ptr<RBX::Reflection::SignalInstance>, 0>::on_tostring(
	const std::shared_ptr<RBX::Reflection::SignalInstance>& value, lua_State* L
)
{
	std::string result("Signal ");
	result += value->descriptor.name.toString();
	lua_pushstring(L, result.c_str());
	return 1;
}

class SignalConnectionBridge : public Bridge<Signals::Connection, 1>
{
	friend class Bridge<Signals::Connection, 1>;

private:
	static int disconnect(lua_State* L);
};

class SignalBridge : public Bridge<std::shared_ptr<RBX::Reflection::SignalInstance>, 0>
{
public:
	static int connect(lua_State* L);
	static int wait(lua_State* L);
};

} // namespace Lua
} // namespace RBX

#endif // SCRIPT_LUASIGNALBRIDGE_H
