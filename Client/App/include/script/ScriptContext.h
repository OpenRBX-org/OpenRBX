#ifndef SCRIPT_SCRIPTCONTEXT_H
#define SCRIPT_SCRIPTCONTEXT_H

#include "reflection/property.h"
#include "reflection/type.h"
#include "script/ThreadRef.h"
#include "security/SecurityContext.h"
#include "util/Handle.h"
#include "util/RunStateOwner.h"
#include "v8datamodel/Stats.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"

#include <chrono>
#include <functional>
#include <lua.h>
#include <memory>
#include <set>
#include <vector>

class LuaAllocator;
struct lua_State;

namespace RBX {

class Script;

namespace Lua {
class YieldingThreads;
}

extern char sScriptContext[];
class ScriptContext : public DescribedCreatable<ScriptContext, Instance, sScriptContext>,
					  public Listener<RunService, RunTransition>,
					  public Listener<RunService, Heartbeat>,
					  public Service
{
public:
	ScriptContext();
	~ScriptContext() override;

	enum Result
	{
		Success,
		Yield,
		Error
	};

	std::unique_ptr<std::vector<Reflection::Value>> call(
		const Lua::FunctionRef&, const std::vector<Reflection::Value>&
	);
	void call(const Lua::FunctionRef&);
	void call(
		const Lua::FunctionRef&, std::function<unsigned int(lua_State*)>, std::function<void(lua_State*, unsigned int)>
	);

	std::unique_ptr<std::vector<Reflection::Value>> executeInNewThread(
		Security::Identities identity,
		const char* script,
		const char* name,
		const std::vector<Reflection::Value>& arguments
	);
	void executeInNewThread(Security::Identities, const char*, const char*);
	void executeInNewThread(
		Security::Identities identity,
		const char* script,
		const char* name,
		std::function<unsigned int(lua_State*)> pushArguments,
		std::function<void(lua_State*, unsigned int)> readResults
	);

	Result resume(lua_State* thread, int narg);
	void gc();
	void resume(
		lua_State* thread,
		std::function<unsigned int(lua_State*)> pushArguments,
		std::function<void(lua_State*, unsigned int)> readResults
	);

	static Reflection::BoundProp<bool, 1> propScriptsDisabled;

	static ScriptContext& getContext(lua_State* L);

	XmlElement* write() override { return nullptr; }

	unsigned int getThreadCount() const;
	bool hasState() const { return globalState != nullptr; }

	lua_State* state();

	void closeState();
	void addScript(Script* script);
	void removeScript(Script* script);

protected:
	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;
	void onEvent(const RunService* source, RunTransition event) override;
	void onEvent(const RunService* source, Heartbeat event) override;

private:
	static void sandboxThread(lua_State* thread);
	static void setThreadIdentity(lua_State* thread, Security::Identities identity);

	static Security::Identities getThreadIdentity(lua_State* thread)
	{
		lua_pushlightuserdata(thread, (void*) 0x54);
		lua_gettable(thread, LUA_GLOBALSINDEX);

		if (!lua_isnumber(thread, -1)) {
			return Security::Anonymous;
		}

		Security::Identities identity = static_cast<Security::Identities>(lua_tointeger(thread, -1));
		lua_settop(thread, -2);

		return identity;
	}

	static int print(lua_State* L);
	static int tick(lua_State* L);
	static int wait(lua_State* thread);
	static int delay(lua_State* thread);
	static int trustedThread(lua_State* thread);
	static int loadfile(lua_State* L);
	static int settings(lua_State* thread);
	static int stats(lua_State* L);
	static int statsitemvalue(lua_State*);
	static int onError(lua_State*);
	static int dofile(lua_State* L);

	static void reportError(lua_State* thread);

	void onChangedScriptEnabled(const Reflection::PropertyDescriptor&);

	void disassociateState(Script* script);
	void openState();
	void startScript(std::shared_ptr<Script> script);
	void startPendingScripts();

	lua_State* globalState;
	Lua::ThreadRef commandLineSandbox;
	std::set<Script*> scripts;
	std::chrono::steady_clock::time_point nextPendingScripts;
	std::vector<std::shared_ptr<Script>> pendingScripts;
	std::shared_ptr<RunService> runService;
	std::unique_ptr<Lua::YieldingThreads> yieldEvent;
	bool scriptsDisabled;
	std::shared_ptr<Stats::Item> statsItem;

	class ScriptImpersonator : public Security::Impersonator
	{
	public:
		ScriptImpersonator(lua_State* thread);
	};

public:
	std::unique_ptr<LuaAllocator> allocator;
};

} // namespace RBX

#endif // SCRIPT_SCRIPTCONTEXT_H
