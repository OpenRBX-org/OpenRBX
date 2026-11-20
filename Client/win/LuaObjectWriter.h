#ifndef LUAOBJECTWRITER_H
#define LUAOBJECTWRITER_H

#include <ostream>
#include <string>

namespace RBX::LDraw2Lua {

class LuaObjectWriter
{
public:
	virtual ~LuaObjectWriter();
	virtual bool write(std::ostream& stream, std::string parent) = 0;
};

} // namespace RBX::LDraw2Lua

#endif // LUAOBJECTWRITER_H
