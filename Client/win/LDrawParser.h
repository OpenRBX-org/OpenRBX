#ifndef LDRAWPARSER_H
#define LDRAWPARSER_H

#include <ostream>
#include <string>

namespace RBX::LDraw2Lua {

class LDrawParser
{
public:
	LDrawParser(const char* fileName);

	virtual ~LDrawParser();

	void writeLua(std::ostream& stream);

	std::string m_fileName;
};

} // namespace RBX::LDraw2Lua

#endif // LDRAWPARSER_H
