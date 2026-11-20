#ifndef LDRAW2ROBLOXMAPROOT_H
#define LDRAW2ROBLOXMAPROOT_H

#include "LDraw2RobloxColorMap.h"
#include "LDraw2RobloxPartMap.h"

#include <map>
#include <string>

namespace RBX::LDraw2Lua {

class LDraw2RobloxMapRoot
{
public:
	LDraw2RobloxMapRoot(std::string fileName);

	virtual ~LDraw2RobloxMapRoot();

	int parseFidelity(std::string text);
	int parseShape(std::string text);
	int parseFormFactor(std::string text);
	LDraw2RobloxColorMap* findColorMap(int code);
	int getRobloxColor(int code);
	LDraw2RobloxPartMap* findPartMap(const std::string& key);
	LDraw2RobloxPartMap* lookupPartMap(const std::string& fileName);
	void addPartMap(LDraw2RobloxPartMap* partMap);

	std::string m_fileName;
	std::string m_schemaVersion;
	std::map<int, LDraw2RobloxColorMap*> m_colorMaps;
	std::map<std::string, LDraw2RobloxPartMap*> m_partMaps;
};

} // namespace RBX::LDraw2Lua

#endif // LDRAW2ROBLOXMAPROOT_H
