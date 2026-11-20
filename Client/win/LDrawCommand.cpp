#include "LDrawCommand.h"

#include "LDraw2RobloxMapRoot.h"

#include <cstdlib>

namespace RBX::LDraw2Lua {

LuaObjectWriter::~LuaObjectWriter() = default;

LDrawCommand::LDrawCommand(LDraw2RobloxMapRoot* mapRoot) : m_mapRoot(mapRoot)
{
	m_type = 0;
	m_color = 0;
	m_transform[0] = 0.0;
	m_transform[1] = 0.0;
	m_transform[2] = 0.0;
	m_transform[3] = 0.0;
	m_transform[4] = 0.0;
	m_transform[5] = 0.0;
	m_transform[6] = 0.0;
	m_transform[7] = 0.0;
	m_transform[8] = 0.0;
	m_transform[9] = 0.0;
	m_transform[10] = 0.0;
	m_transform[11] = 0.0;

	std::string text("");
}

LDrawCommand::~LDrawCommand() = default;

bool LDrawCommand::writePart(std::ostream& stream, const RobloxPart* part)
{
	stream << "-- " << "Part Description -------------------------------------" << "\n";
	stream << "part = Instance.new(\"Part\")" << "\n";

	std::string name = m_fileName.substr(0, m_fileName.find_first_of(".", 0));
	stream << "part.Name = \"" << name.c_str() << "\"" << "\n";

	int shape = part->m_shape;
	stream << "part.Shape = " << shape << "\n";

	if (shape == 0 || shape == 2) {
		stream << "part.FormFactor = 0  -- symmetric" << "\n";
	}
	else {
		stream << "part.FormFactor = " << part->m_formFactor << "\n";
	}

	stream << "part.Size = Vector3.new(" << part->m_size.c_str() << ")" << "\n";
	return true;
}

bool LDrawCommand::writeColor(std::ostream& stream)
{
	stream << "part.BrickColor = BrickColor.new( " << m_mapRoot->getRobloxColor(m_color) << " )";
	stream << "\n";
	return true;
}

bool LDrawCommand::writeCFrame(std::ostream& stream, std::string yOffset)
{
	double offset = atof(yOffset.c_str());
	double x = m_transform[0] / 20.0;
	double y = m_transform[1] / -20.0 - offset;
	double z = m_transform[2] / -20.0;
	double r00 = m_transform[3];
	double r01 = m_transform[4];
	double r02 = m_transform[5];
	double r10 = m_transform[6];
	double r11 = m_transform[7];
	double r12 = m_transform[8];
	double r20 = m_transform[9];
	double r21 = m_transform[10];
	double r22 = m_transform[11];

	stream << "part.CFrame = CFrame.new(" << x << "," << y << "," << z << "," << r00 << "," << r01 << "," << r02 << ","
		   << r10 << "," << r11 << "," << r12 << "," << r20 << "," << r21 << "," << r22 << ")";
	stream << "\n";
	return false;
}

bool LDrawCommand::writeParent(std::ostream& stream, std::string parent)
{
	stream << "part.Parent = " << parent.c_str();
	stream << "\n";
	stream << "part:MakeJoints()";
	stream << "\n";
	return false;
}

bool LDrawCommand::write(std::ostream& stream, std::string parent)
{
	if (m_type == 1) {
		LDraw2RobloxPartMap* map = m_mapRoot->lookupPartMap(m_fileName);
		if (map) {
			const RobloxPart* part = map->getFirstRobloxPart();
			if (part) {
				writePart(stream, part);
				writeColor(stream);
				writeCFrame(stream, part->m_yOffset);
				writeParent(stream, parent);
				stream << "\n";
				return true;
			}
		}
	}

	return false;
}

} // namespace RBX::LDraw2Lua
