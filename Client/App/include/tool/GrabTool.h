#ifndef TOOL_GRABTOOL_H
#define TOOL_GRABTOOL_H

#include "util/Name.h"
#include "v8datamodel/MouseCommand.h"

#include <string>

namespace RBX {

class Workspace;

extern char sGrabTool[];

class GrabTool : public Named<MouseCommand, sGrabTool>
{
private:
	std::string cursor;

	void onMouseIdle(const UIEvent& uiEvent) override { onMouseHover(uiEvent); }
	void onMouseHover(const UIEvent& uiEvent) override;
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;

	const std::string getCursorName() const override { return cursor; }

public:
	GrabTool(Workspace* workspace);
	~GrabTool() override;

	MouseCommand* isSticky() const override { return new GrabTool(workspace); }
};

} // namespace RBX

#endif // TOOL_GRABTOOL_H
