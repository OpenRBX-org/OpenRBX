#ifndef V8DATAMODEL_IEQUIPABLE_H
#define V8DATAMODEL_IEQUIPABLE_H

#include <G3D/CoordinateFrame.h>
#include <memory>
#include <string>

namespace RBX {

class PartInstance;
class Weld;
class Workspace;

using G3D::CoordinateFrame;

class IEquipable
{
protected:
	std::shared_ptr<Weld> weld;
	Workspace* workspace;

	void buildWeld(
		PartInstance* humanoidPart,
		PartInstance* gadgetPart,
		const CoordinateFrame& humanoidCoord,
		const CoordinateFrame& gadgetCoord,
		const std::string& name
	);

	IEquipable();

	virtual ~IEquipable();
};

} // namespace RBX

#endif // V8DATAMODEL_IEQUIPABLE_H
