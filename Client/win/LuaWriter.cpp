#include "LuaWriter.h"

#include "LDrawCommand.h"

#include <cctype>

namespace RBX::LDraw2Lua {

LuaWriter::LuaWriter(const char* modelName) : m_modelName(modelName)
{
}

LuaWriter::~LuaWriter()
{
	size_t count = m_objects.size();
	for (size_t i = 0; i < count; i++) {
		delete m_objects.at(i);
	}

	m_objects.clear();
}

void LuaWriter::write(std::ostream& stream)
{
	if (m_objects.size() > 0) {
		stream << "-- " << "Get the current workspace" << "\n";
		stream << "local w = game:service(\"Workspace\")" << "\n";
		stream << "\n";
		stream << "-- " << "Create a new model" << "\n";
		stream << "m = Instance.new(\"Model\")" << "\n";
		stream << "m.Name = \"" << m_modelName.c_str() << "\"" << "\n";
		stream << "m.Parent = w" << "\n";
		stream << "\n";

		size_t count = m_objects.size();
		for (size_t i = 0; i < count; i++) {
			m_objects.at(i)->write(stream, "m");
		}
	}
}

void LuaWriter::add(LDrawCommand* command)
{
	m_objects.push_back(command);
}

} // namespace RBX::LDraw2Lua

int compareNoCase(const std::string& left, const std::string& right)
{
	std::string::const_iterator p = left.begin();
	std::string::const_iterator p2 = right.begin();

	while (p != left.end() && p2 != right.end()) {
		if (toupper(*p) != toupper(*p2)) {
			return toupper(*p) < toupper(*p2) ? -1 : 1;
		}

		++p;
		++p2;
	}

	size_t size = left.size();
	size_t size2 = right.size();
	return size2 == size ? 0 : size < size2 ? -1 : 1;
}
