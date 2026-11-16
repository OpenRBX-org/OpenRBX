#ifndef LDRAWPARSER_H
#define LDRAWPARSER_H

#include <ostream>
#include <string>

namespace RBX::LDraw2Lua {

class LDrawParser
{
public:
	LDrawParser(const char* p_unk0x04);

	virtual ~LDrawParser();

	void FUN_00473a80(std::ostream& stream);

	std::string m_unk0x04;
};

} // namespace RBX::LDraw2Lua

#endif // LDRAWPARSER_H
