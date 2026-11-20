#ifndef LDRAW2ROBLOXPARTMAP_H
#define LDRAW2ROBLOXPARTMAP_H

#include <string>
#include <vector>

namespace RBX::LDraw2Lua {

struct RobloxPart
{
	RobloxPart(int shape, int formFactor, std::string size, std::string yOffset);

	int m_shape;
	int m_formFactor;
	std::string m_size;
	std::string m_yOffset;
};

class LDraw2RobloxPartMap
{
public:
	LDraw2RobloxPartMap(std::string name, std::string description, std::string ldrawFilename, int fidelity);

	virtual ~LDraw2RobloxPartMap();

	std::string getLDrawFilename() const { return m_ldrawFilename; }

	const RobloxPart* getFirstRobloxPart() const;
	void addRobloxPart(const RobloxPart& part);

	std::string m_name;
	std::string m_description;
	std::string m_ldrawFilename;
	int m_fidelity;
	std::vector<RobloxPart> m_robloxParts;
};

} // namespace RBX::LDraw2Lua

#endif // LDRAW2ROBLOXPARTMAP_H
