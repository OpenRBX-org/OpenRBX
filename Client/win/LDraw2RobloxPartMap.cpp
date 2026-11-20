#include "LDraw2RobloxPartMap.h"

namespace RBX::LDraw2Lua {

const RobloxPart* LDraw2RobloxPartMap::getFirstRobloxPart() const
{
	if (m_robloxParts.size() > 0) {
		return &m_robloxParts.at(0);
	}

	return nullptr;
}

LDraw2RobloxPartMap::LDraw2RobloxPartMap(
	std::string name, std::string description, std::string ldrawFilename, int fidelity
)
	: m_name(name), m_description(description), m_ldrawFilename(ldrawFilename), m_fidelity(fidelity)
{
}

LDraw2RobloxPartMap::~LDraw2RobloxPartMap() = default;

void LDraw2RobloxPartMap::addRobloxPart(const RobloxPart& part)
{
	m_robloxParts.push_back(part);
}

RobloxPart::RobloxPart(int shape, int formFactor, std::string size, std::string yOffset)
	: m_shape(shape), m_formFactor(formFactor), m_size(size), m_yOffset(yOffset)
{
}

} // namespace RBX::LDraw2Lua
