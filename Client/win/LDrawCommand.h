#ifndef LDRAWCOMMAND_H
#define LDRAWCOMMAND_H

#include "LuaObjectWriter.h"

#include <ostream>
#include <string>

namespace RBX::LDraw2Lua {

class LDraw2RobloxMapRoot;
struct Unknown00473110;

class LDrawCommand : public LuaObjectWriter
{
public:
	LDrawCommand(LDraw2RobloxMapRoot* p_unk0x04);

	~LDrawCommand() override;
	bool VTable0x04(std::ostream& stream, std::string p_unk0x08) override;

	void setFileName(std::string p_unk0x04) { m_unk0x70 = p_unk0x04; }

	bool FUN_00473330(std::ostream& stream, const Unknown00473110* p_unk0x08);
	bool FUN_00473500(std::ostream& stream);
	bool FUN_00473550(std::ostream& stream, std::string p_unk0x08);
	bool FUN_00473780(std::ostream& stream, std::string p_unk0x08);

	LDraw2RobloxMapRoot* m_unk0x04;
	int m_unk0x08;
	int m_unk0x0c;
	double m_unk0x10[12];
	std::string m_unk0x70;
};

} // namespace RBX::LDraw2Lua

#endif // LDRAWCOMMAND_H
