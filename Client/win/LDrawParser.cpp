#include "LDrawParser.h"

#include "LDraw2RobloxMapRoot.h"
#include "LDrawCommand.h"
#include "LuaWriter.h"
#include "util/ContentProvider.h"

#include <fstream>
#include <memory>

namespace RBX::LDraw2Lua {

static std::unique_ptr<LDraw2RobloxMapRoot> mapRoot;

LDrawParser::~LDrawParser() = default;

LDrawParser::LDrawParser(const char* fileName) : m_fileName(fileName)
{
	if (!mapRoot.get()) {
		mapRoot.reset(
			new LDraw2RobloxMapRoot(RBX::ContentProvider::singleton().getAssetFile("fonts\\LDraw2RobloxMap.xml"))
		);
	}
}

void LDrawParser::writeLua(std::ostream& stream)
{
	stream << "-- " << "Roblox LDraw2Lua Conversion" << "\n";
	stream << "-- " << "FileName: " << m_fileName.c_str() << "\n";
	stream << "\n";

	LuaWriter writer("LDrawModel");

	{
		std::ifstream input(m_fileName.c_str());

		while (!input.eof()) {
			char line[2048];
			int type;
			input >> type;

			if (type == 1) {
				LDrawCommand* command = new LDrawCommand(mapRoot.get());
				command->m_type = 1;

				int color = -1;
				input >> color;
				command->m_color = color;

				double value = 0.0;
				input >> value;
				command->m_transform[0] = value;
				input >> value;
				command->m_transform[1] = value;
				input >> value;
				command->m_transform[2] = value;
				input >> value;
				command->m_transform[3] = value;
				input >> value;
				command->m_transform[4] = value;
				input >> value;
				command->m_transform[5] = value;
				input >> value;
				command->m_transform[6] = value;
				input >> value;
				command->m_transform[7] = value;
				input >> value;
				command->m_transform[8] = value;
				input >> value;
				command->m_transform[9] = value;
				input >> value;
				command->m_transform[10] = value;
				input >> value;
				command->m_transform[11] = value;

				input.getline(line, 2000);
				std::string text(line);
				command->setFileName(text.substr(text.find_first_not_of(" ", 0), text.length()));
				writer.add(command);
			}
			else {
				input.getline(line, 2000);
			}
		}
	}

	writer.write(stream);
}

} // namespace RBX::LDraw2Lua
