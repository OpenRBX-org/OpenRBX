#include "script/ThreadRef.h"

#include "lua/LuaBridge.h"
#include "reflection/type.h"

#include <lauxlib.h>
#include <lua.h>

namespace RBX {
namespace Lua {

std::shared_ptr<std::recursive_mutex> ThreadRef::syncSingleton(new std::recursive_mutex());

template <>
const char* Bridge<std::shared_ptr<ThreadRef::Node>, 1>::className = "shared_ptr<ThreadRef::Node>";

void ThreadRef::addRef()
{
	if (L != nullptr) {
		lua_pushthread(L);

		threadId = luaL_ref(L, LUA_REGISTRYINDEX);
	}
}

void ThreadRef::addToNode()
{
	if (node != nullptr) {
		ThreadRef* first = node->first;

		if (first != nullptr) {
			next = first;
			first->previous = this;
		}
		else {
			next = nullptr;
		}

		previous = nullptr;
		node->first = this;
	}
}

void ThreadRef::removeRef()
{
	if (L != nullptr) {
		luaL_unref(L, LUA_REGISTRYINDEX, threadId);

		threadId = 0;
		L = nullptr;
	}
}

void ThreadRef::removeFromNode()
{
	if (node != nullptr) {
		if (next != nullptr) {
			next->previous = previous;
		}

		if (previous != nullptr) {
			previous->next = next;
		}

		if (node->first == this) {
			node->first = next;
		}

		next = nullptr;
		previous = nullptr;
		node = nullptr;
	}
}

template <>
void Bridge<std::shared_ptr<ThreadRef::Node>, 1>::on_newindex(
	std::shared_ptr<ThreadRef::Node>& value, const char* name, lua_State* L
)
{
	throw std::runtime_error(G3D::format("%s cannot be assigned to", name));
}

template <>
int Bridge<std::shared_ptr<ThreadRef::Node>, 1>::on_index(
	const std::shared_ptr<ThreadRef::Node>& value, const char* name, lua_State* L
)
{
	throw std::runtime_error(G3D::format("%s is not a valid member", name));
}

void lua_pushfunction(lua_State* L, const FunctionRef& function)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, function.functionId);
}

void FunctionRef::removeRef()
{
	if (functionId != 0 && thread() != nullptr) {
		luaL_unref(thread(), LUA_REGISTRYINDEX, functionId);

		functionId = 0;
	}

	ThreadRef::removeRef();
}

std::shared_ptr<ThreadRef::Node> ThreadRef::Node::get(lua_State* thread)
{
	lua_pushlightuserdata(thread, (void*) 0x4e);
	lua_gettable(thread, LUA_GLOBALSINDEX);

	std::shared_ptr<Node>& node = Bridge<std::shared_ptr<Node>, 1>::getObject(thread, lua_gettop(thread));

	lua_settop(thread, -2);

	return node;
}

} // namespace Lua

namespace Reflection {

template <>
const Type& Type::singleton<Lua::FunctionRef>()
{
	static Type type("Function", typeid(Lua::FunctionRef));

	return type;
}

} // namespace Reflection

namespace Lua {

ThreadRef::ThreadRef(lua_State* L)
	: sync(syncSingleton), node(nullptr), previous(nullptr), next(nullptr), L(L), threadId(0)
{
	std::scoped_lock lock(*sync);

	node = Node::get(L).get();

	addToNode();

	addRef();
}

ThreadRef::ThreadRef(const ThreadRef& other) : sync(syncSingleton), node(other.node), L(other.L)
{
	std::scoped_lock lock(*sync);

	addRef();

	addToNode();
}

void ThreadRef::reset()
{
	std::scoped_lock lock(*sync);

	removeFromNode();

	removeRef();
}

ThreadRef& ThreadRef::operator=(const ThreadRef& other)
{
	if (L != other.L) {
		std::scoped_lock lock(*sync);

		removeRef();

		L = other.L;

		addRef();
	}

	if (node != other.node) {
		std::scoped_lock lock(*sync);

		removeFromNode();

		node = other.node;

		addToNode();
	}

	return *this;
}

void ThreadRef::Node::eraseAllRefs()
{
	std::scoped_lock lock(*sync);

	ThreadRef* ref = first;

	while (ref != nullptr) {
		ref->removeRef();
		ref->node = nullptr;

		ref = ref->next;
	}

	first = nullptr;
}

ThreadRef::Node::~Node()
{
	eraseAllRefs();
}

FunctionRef& FunctionRef::operator=(const FunctionRef& other)
{
	if (thread() != nullptr) {
		luaL_unref(thread(), LUA_REGISTRYINDEX, functionId);
	}

	ThreadRef::operator=(other);

	if (thread() == nullptr) {
		functionId = 0;
	}
	else {
		lua_rawgeti(thread(), LUA_REGISTRYINDEX, other.functionId);

		functionId = luaL_ref(thread(), LUA_REGISTRYINDEX);
	}

	return *this;
}

ThreadRef::~ThreadRef()
{
	reset();
}

FunctionRef::FunctionRef(lua_State* L, int index) : ThreadRef(L)
{
	lua_pushvalue(L, index);

	functionId = luaL_ref(L, LUA_REGISTRYINDEX);
}

FunctionRef::~FunctionRef()
{
	if (functionId != 0 && thread() != nullptr) {
		luaL_unref(thread(), LUA_REGISTRYINDEX, functionId);
	}
}

FunctionRef::FunctionRef(const FunctionRef& other) : ThreadRef(other)
{
	if (thread() == nullptr) {
		functionId = 0;
	}
	else {
		lua_rawgeti(thread(), LUA_REGISTRYINDEX, other.functionId);

		functionId = luaL_ref(thread(), LUA_REGISTRYINDEX);
	}
}

FunctionRef lua_tofunction(lua_State* L, int index)
{
	return FunctionRef(L, index);
}

} // namespace Lua

namespace Reflection {

template <>
Lua::FunctionRef& Value::convert<Lua::FunctionRef>()
{
	if (isType<void>()) {
		value = Lua::FunctionRef();
		_type = &Type::singleton<Lua::FunctionRef>();
	}

	if (Lua::FunctionRef* function = std::any_cast<Lua::FunctionRef>(&value)) {
		return *function;
	}

	throw std::runtime_error("Unable to cast value to RBX::Lua::FunctionRef");
}

} // namespace Reflection

namespace Lua {

std::shared_ptr<ThreadRef::Node> ThreadRef::Node::create(lua_State* thread)
{
	lua_pushlightuserdata(thread, (void*) 0x4e);

	std::shared_ptr<Node>& node = Bridge<std::shared_ptr<Node>, 1>::pushNewObject(thread);

	lua_settable(thread, LUA_GLOBALSINDEX);

	node = std::shared_ptr<Node>(new Node());

	return node;
}

} // namespace Lua
} // namespace RBX
