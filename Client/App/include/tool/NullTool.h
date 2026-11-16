#ifndef TOOL_NULLTOOL_H
#define TOOL_NULLTOOL_H

#include "util/Name.h"
#include "v8datamodel/MouseCommand.h"

#include <G3D/Vector3.h>
#include <string>

namespace RBX {

class Workspace;

extern char sNullTool[];
extern char sNewNullTool[];

class NullTool : public Named<MouseCommand, sNullTool>
{
private:
	const std::string getCursorName() const override { return "ArrowFarCursor"; }

public:
	NullTool(Workspace* workspace);
	~NullTool() override = default;

	MouseCommand* isSticky() const override { return new NullTool(workspace); }

	MouseCommand* onMouseUp(const UIEvent& uiEvent) override
	{
		releaseCapture();
		return this;
	}
};

class NewNullTool : public Named<MouseCommand, sNewNullTool>
{
private:
	std::string cursor;
	bool hasWaypoint;
	G3D::Vector3 waypoint;

	void onMouseIdle(const UIEvent& uiEvent) override;
	void onMouseHover(const UIEvent& uiEvent) override;
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	const std::string getCursorName() const override { return cursor; }
	MouseCommand* isSticky() const override { return new NewNullTool(workspace); }
	MouseCommand* onMouseUp(const UIEvent& uiEvent) override
	{
		releaseCapture();
		return this;
	}
	bool shouldRender3dAdorn() const override { return true; }
	void render3dAdorn(Adorn* adorn) override;

public:
	NewNullTool(Workspace* workspace);
	~NewNullTool() override;
};

} // namespace RBX

#endif // TOOL_NULLTOOL_H
