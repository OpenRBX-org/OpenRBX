#ifndef LDRAW2ROBLOXCOLORMAP_H
#define LDRAW2ROBLOXCOLORMAP_H

namespace RBX::LDraw2Lua {

class LDraw2RobloxColorMap
{
public:
	LDraw2RobloxColorMap(int p_unk0x04, int p_unk0x08);

	virtual ~LDraw2RobloxColorMap() = default;

	int m_unk0x04;
	int m_unk0x08;
};

} // namespace RBX::LDraw2Lua

#endif // LDRAW2ROBLOXCOLORMAP_H
