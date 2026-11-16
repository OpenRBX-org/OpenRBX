#ifndef TOOL_PARTDRAGTOOL_H
#define TOOL_PARTDRAGTOOL_H

#include "tool/MegaDragger.h"
#include "tool/RunDragger.h"
#include "util/Name.h"
#include "v8datamodel/MouseCommand.h"

#include <G3D/Vector2.h>
#include <G3D/Vector3.h>
#include <memory>
#include <string>

namespace RBX {

class Adorn;
class PartInstance;
class Workspace;

extern char sPartDragTool[];
class PartDragTool : public Named<MouseCommand, sPartDragTool>
{
private:
	std::unique_ptr<RunDragger> runDragger;
	std::unique_ptr<MegaDragger> megaDragger;
	G3D::Vector2 downPoint;
	bool dragging;
	G3D::Vector3 hitWorld;

public:
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseIdle(const UIEvent& uiEvent) override;
	void onMouseMove(const UIEvent& uiEvent) override;
	void onMouseDelta(const UIEvent& uiEvent) override;
	MouseCommand* onMouseUp(const UIEvent& uiEvent) override;
	MouseCommand* onKeyDown(const UIEvent& uiEvent) override;
	void render3dAdorn(Adorn* adorn) override;

	const std::string getCursorName() const override { return dragging ? "GrabRotateCursor" : "DragCursor"; }

	PartDragTool(PartInstance* mousePart, const G3D::Vector3& hitPointWorld, Workspace* workspace);
	~PartDragTool() override;
};

} // namespace RBX

#endif // TOOL_PARTDRAGTOOL_H
