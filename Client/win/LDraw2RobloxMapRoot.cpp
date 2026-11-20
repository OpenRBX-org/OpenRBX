#include "LDraw2RobloxMapRoot.h"

#include "LuaWriter.h"
#include "util/Name.h"
#include "v8xml/XmlElement.h"
#include "v8xml/XmlSerializer.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <memory>

namespace RBX::LDraw2Lua {

static const RBX::Name& tagLDraw2RobloxMap = RBX::Name::declare("LDraw2RobloxMap", -1);
static const RBX::Name& tagSchemaVersion = RBX::Name::declare("schema_version", -1);
static const RBX::Name& tagColorMap = RBX::Name::declare("color_map", -1);
static const RBX::Name& tagColor = RBX::Name::declare("color", -1);
static const RBX::Name& tagLDrawCode = RBX::Name::declare("ldraw_code", -1);
static const RBX::Name& tagRobloxCode = RBX::Name::declare("roblox_code", -1);
static const RBX::Name& tagPartMap = RBX::Name::declare("part_map", -1);
static const RBX::Name& tagPart = RBX::Name::declare("part", -1);
static const RBX::Name& tagName = RBX::Name::declare("name", -1);
static const RBX::Name& tagDescription = RBX::Name::declare("description", -1);
static const RBX::Name& tagLDrawFilename = RBX::Name::declare("ldraw_filename", -1);
static const RBX::Name& tagMappingFidelity = RBX::Name::declare("mapping_fidelity", -1);
static const RBX::Name& tagRobloxPart = RBX::Name::declare("roblox_part", -1);
static const RBX::Name& tagShape = RBX::Name::declare("shape", -1);
static const RBX::Name& tagFormFactor = RBX::Name::declare("form_factor", -1);
static const RBX::Name& tagSizeVect3 = RBX::Name::declare("size_vect3", -1);
static const RBX::Name& tagYOffset = RBX::Name::declare("y_offset", -1);

int LDraw2RobloxMapRoot::parseFidelity(std::string text)
{
	if (compareNoCase(text, "exact") == 0) {
		return 0;
	}

	if (compareNoCase(text, "close") == 0) {
		return 1;
	}

	return 2;
}

int LDraw2RobloxMapRoot::parseShape(std::string text)
{
	if (compareNoCase(text, "ball") == 0) {
		return 0;
	}

	if (compareNoCase(text, "block") == 0) {
		return 1;
	}

	if (compareNoCase(text, "cylinder") == 0) {
		return 2;
	}

	return 999;
}

int LDraw2RobloxMapRoot::parseFormFactor(std::string text)
{
	if (compareNoCase(text, "symmetric") == 0) {
		return 0;
	}

	if (compareNoCase(text, "brick") == 0) {
		return 1;
	}

	return 2;
}

LDraw2RobloxColorMap* LDraw2RobloxMapRoot::findColorMap(int code)
{
	std::map<int, LDraw2RobloxColorMap*>::iterator iter = m_colorMaps.find(code);
	if (iter != m_colorMaps.end()) {
		return iter->second;
	}

	return nullptr;
}

int LDraw2RobloxMapRoot::getRobloxColor(int code)
{
	LDraw2RobloxColorMap* color = findColorMap(code);
	if (color) {
		return color->m_robloxCode;
	}

	return 1;
}

LDraw2RobloxPartMap* LDraw2RobloxMapRoot::findPartMap(const std::string& key)
{
	std::map<std::string, LDraw2RobloxPartMap*>::iterator iter = m_partMaps.find(key);
	if (iter != m_partMaps.end()) {
		return iter->second;
	}

	return nullptr;
}

LDraw2RobloxPartMap* LDraw2RobloxMapRoot::lookupPartMap(const std::string& fileName)
{
	std::string key = fileName;
	std::transform(key.begin(), key.end(), key.begin(), tolower);
	return findPartMap(key);
}

void LDraw2RobloxMapRoot::addPartMap(LDraw2RobloxPartMap* partMap)
{
	std::string key = partMap->getLDrawFilename();
	std::transform(key.begin(), key.end(), key.begin(), tolower);

	LDraw2RobloxPartMap* part = findPartMap(key);
	if (part) {
		delete part;
	}

	m_partMaps[key] = partMap;
}

LDraw2RobloxMapRoot::~LDraw2RobloxMapRoot()
{
	for (std::map<int, LDraw2RobloxColorMap*>::const_iterator color = m_colorMaps.begin(); color != m_colorMaps.end();
		 ++color) {
		delete color->second;
	}

	for (std::map<std::string, LDraw2RobloxPartMap*>::const_iterator part = m_partMaps.begin();
		 part != m_partMaps.end();
		 ++part) {
		delete part->second;
	}
}

LDraw2RobloxMapRoot::LDraw2RobloxMapRoot(std::string fileName) : m_fileName(fileName)
{
	std::ifstream stream(m_fileName.c_str());
	TextXmlParser parser(stream.rdbuf());
	std::unique_ptr<XmlElement> root = parser.parse();

	const XmlElement* element = root->findFirstChildByTag(tagSchemaVersion);
	if (element) {
		element->getValue(m_schemaVersion);
	}

	element = root->findFirstChildByTag(tagColorMap);
	if (element) {
		element = element->findFirstChildByTag(tagColor);
		std::string text;

		while (element) {
			int count = 0;
			element->findFirstChildByTag(tagName);

			const XmlElement* ldraw = element->findFirstChildByTag(tagLDrawCode);
			int ldrawCode = -1;
			int robloxCode = -1;
			if (ldraw) {
				count++;
				ldraw->getValue(text);
				ldrawCode = atoi(text.c_str());
			}

			const XmlElement* roblox = element->findFirstChildByTag(tagRobloxCode);
			if (roblox) {
				count++;
				roblox->getValue(text);
				robloxCode = atoi(text.c_str());
			}

			if (count == 2) {
				LDraw2RobloxColorMap* map = new LDraw2RobloxColorMap(ldrawCode, robloxCode);
				LDraw2RobloxColorMap* old = findColorMap(map->m_ldrawCode);
				if (old) {
					delete old;
				}

				int code = map->m_ldrawCode;
				m_colorMaps[code] = map;
			}

			element = element->nextSibling();
		}
	}

	const XmlElement* parts = root->findFirstChildByTag(tagPartMap);
	if (parts) {
		const XmlElement* part = parts->findFirstChildByTag(tagPart);
		while (part) {
			int count = 0;

			const XmlElement* value = part->findFirstChildByTag(tagName);
			std::string name("");
			if (value) {
				count++;
				value->getValue(name);
			}

			value = part->findFirstChildByTag(tagDescription);
			std::string description("");
			if (value) {
				count++;
				value->getValue(description);
			}

			value = part->findFirstChildByTag(tagLDrawFilename);
			std::string ldrawFilename("");
			if (value) {
				count++;
				value->getValue(ldrawFilename);
			}

			value = part->findFirstChildByTag(tagMappingFidelity);
			std::string mappingFidelity("");
			int fidelity = 2;
			if (value) {
				count++;
				value->getValue(mappingFidelity);
				fidelity = parseFidelity(mappingFidelity);
			}

			LDraw2RobloxPartMap* partMap = new LDraw2RobloxPartMap(name, description, ldrawFilename, fidelity);

			const XmlElement* robloxPart = part->findFirstChildByTag(tagRobloxPart);
			if (robloxPart) {
				count++;

				while (robloxPart) {
					int partCount = 0;

					const XmlElement* item = robloxPart->findFirstChildByTag(tagShape);
					std::string text("");
					int shape = 999;
					if (item) {
						partCount++;
						item->getValue(text);
						shape = parseShape(text);
					}

					item = robloxPart->findFirstChildByTag(tagFormFactor);
					text = "";
					int formFactor = 0;
					if (item) {
						partCount++;
						item->getValue(text);
						formFactor = parseFormFactor(text);
					}

					item = robloxPart->findFirstChildByTag(tagSizeVect3);
					std::string size("");
					if (item) {
						partCount++;
						item->getValue(size);
					}

					item = robloxPart->findFirstChildByTag(tagYOffset);
					std::string yOffset("");
					if (item) {
						partCount++;
						item->getValue(yOffset);
					}

					if (partCount == 4) {
						partMap->addRobloxPart(RobloxPart(shape, formFactor, size, yOffset));
					}

					robloxPart = robloxPart->nextSibling();
				}
			}

			if (count == 5) {
				addPartMap(partMap);
			}

			part = part->nextSibling();
		}
	}
}

} // namespace RBX::LDraw2Lua
