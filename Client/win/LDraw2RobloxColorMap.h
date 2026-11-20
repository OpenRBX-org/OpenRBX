#ifndef LDRAW2ROBLOXCOLORMAP_H
#define LDRAW2ROBLOXCOLORMAP_H

namespace RBX::LDraw2Lua {

class LDraw2RobloxColorMap
{
public:
	LDraw2RobloxColorMap(int ldrawCode, int robloxCode);

	virtual ~LDraw2RobloxColorMap() = default;

	int m_ldrawCode;
	int m_robloxCode;
};

} // namespace RBX::LDraw2Lua

#endif // LDRAW2ROBLOXCOLORMAP_H
