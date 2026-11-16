#ifndef LUAOBJECTWRITER_H
#define LUAOBJECTWRITER_H

#include <ostream>
#include <string>

namespace RBX::LDraw2Lua {

class LuaObjectWriter
{
public:
	virtual ~LuaObjectWriter();
	virtual bool VTable0x04(std::ostream& stream, std::string p_unk0x08) = 0;
};

} // namespace RBX::LDraw2Lua

#endif // LUAOBJECTWRITER_H
