#ifndef V8DATAMODEL_TOOLMOUSECOMMAND_H
#define V8DATAMODEL_TOOLMOUSECOMMAND_H

#include "util/Name.h"
#include "v8datamodel/ScriptMouseCommand.h"

#include <memory>

namespace RBX {

class Tool;

extern char sToolMouseCommand[];

class ToolMouseCommand : public Named<ScriptMouseCommand, sToolMouseCommand>
{
private:
	std::shared_ptr<Tool> tool;

	void updateTargetPoint(const UIEvent& uiEvent);

public:
	ToolMouseCommand(Workspace* workspace, Tool* tool)
		: Named<ScriptMouseCommand, sToolMouseCommand>(workspace), tool(shared_from(tool))
	{
	}

	~ToolMouseCommand() override = default;

	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseHover(const UIEvent& uiEvent) override;
	void onMouseIdle(const UIEvent& uiEvent) override;
	MouseCommand* onMouseUp(const UIEvent& uiEvent) override;
};

} // namespace RBX

#endif // V8DATAMODEL_TOOLMOUSECOMMAND_H
