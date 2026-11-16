#ifndef TOOL_GROUPDRAGTOOL_H
#define TOOL_GROUPDRAGTOOL_H

#include "tool/MegaDragger.h"
#include "util/Name.h"
#include "v8datamodel/MouseCommand.h"

#include <G3D/Vector2.h>
#include <G3D/Vector3.h>
#include <memory>
#include <string>
#include <vector>

namespace RBX {

class PartInstance;
class Workspace;

extern char sGroupDragTool[];
class GroupDragTool : public Named<MouseCommand, sGroupDragTool>
{
private:
	std::unique_ptr<MegaDragger> megaDragger;
	G3D::Vector2 downPoint;
	bool dragging;
	G3D::Vector3 lastHit;

public:
	MouseCommand* onKeyDown(const UIEvent& uiEvent) override;
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseIdle(const UIEvent& uiEvent) override;
	void onMouseMove(const UIEvent& uiEvent) override;
	MouseCommand* onMouseUp(const UIEvent& uiEvent) override;

	const std::string getCursorName() const override { return dragging ? "GrabRotateCursor" : "DragCursor"; }

	GroupDragTool(
		PartInstance* mousePart,
		const G3D::Vector3& hitPointWorld,
		const std::vector<std::weak_ptr<PartInstance>>& partArray,
		Workspace* workspace
	);
	~GroupDragTool() override;
};

} // namespace RBX

#endif // TOOL_GROUPDRAGTOOL_H
