#ifndef TOOL_MEGADRAGGER_H
#define TOOL_MEGADRAGGER_H

#include <G3D/Matrix3.h>
#include <G3D/Vector3.h>
#include <memory>
#include <vector>

namespace RBX {

class ContactManager;
class PartInstance;
class PVInstance;
class RootInstance;
class UIEvent;

class MegaDragger
{
private:
	std::weak_ptr<PartInstance> mousePart;
	std::vector<std::weak_ptr<PartInstance> > dragParts;
	bool joined;
	RootInstance* rootInstance;
	ContactManager* contactManager;

public:
	MegaDragger(PartInstance* mousePartPtr, const std::vector<PVInstance*>& pvInstances, RootInstance* rootInstance);
	MegaDragger(
		PartInstance* mousePartPtr,
		const std::vector<std::weak_ptr<PartInstance> >& partArray,
		RootInstance* rootInstance
	);
	~MegaDragger();

	void startDragging();
	void continueDragging();
	void finishDragging();
	bool mousePartAlive();
	bool anyDragPartAlive();

	std::weak_ptr<PartInstance> getMousePart() { return mousePart; }

	void alignAndCleanParts();
	G3D::Vector3 hitObjectOrPlane(const UIEvent& uiEvent);
	G3D::Vector3 safeMoveYDrop(const G3D::Vector3& tryDrag);
	G3D::Vector3 safeMoveAlongLine(const G3D::Vector3&);
	G3D::Vector3 safeMoveNoDrop(const G3D::Vector3& tryDrag);
	void safeRotate(const G3D::Matrix3& rotMatrix);
};

} // namespace RBX

#endif // TOOL_MEGADRAGGER_H
