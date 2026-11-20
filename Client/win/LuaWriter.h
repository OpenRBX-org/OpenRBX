#ifndef LUAWRITER_H
#define LUAWRITER_H

#include "LuaObjectWriter.h"

#include <ostream>
#include <string>
#include <vector>

namespace RBX::LDraw2Lua {

class LDrawCommand;

class LuaWriter
{
public:
	LuaWriter(const char* modelName);

	virtual ~LuaWriter();

	void write(std::ostream& stream);
	void add(LDrawCommand* command);

	std::string m_modelName;
	std::vector<LuaObjectWriter*> m_objects;
};

} // namespace RBX::LDraw2Lua

int compareNoCase(const std::string& left, const std::string& right);

#endif // LUAWRITER_H
