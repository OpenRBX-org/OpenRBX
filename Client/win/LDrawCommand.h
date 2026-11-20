#ifndef LDRAWCOMMAND_H
#define LDRAWCOMMAND_H

#include "LuaObjectWriter.h"

#include <ostream>
#include <string>

namespace RBX::LDraw2Lua {

class LDraw2RobloxMapRoot;
struct RobloxPart;

class LDrawCommand : public LuaObjectWriter
{
public:
	LDrawCommand(LDraw2RobloxMapRoot* mapRoot);

	~LDrawCommand() override;
	bool write(std::ostream& stream, std::string parent) override;

	void setFileName(std::string fileName) { m_fileName = fileName; }

	bool writePart(std::ostream& stream, const RobloxPart* part);
	bool writeColor(std::ostream& stream);
	bool writeCFrame(std::ostream& stream, std::string yOffset);
	bool writeParent(std::ostream& stream, std::string parent);

	LDraw2RobloxMapRoot* m_mapRoot;
	int m_type;
	int m_color;
	double m_transform[12];
	std::string m_fileName;
};

} // namespace RBX::LDraw2Lua

#endif // LDRAWCOMMAND_H
