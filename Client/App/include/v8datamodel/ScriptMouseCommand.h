#ifndef V8DATAMODEL_SCRIPTMOUSECOMMAND_H
#define V8DATAMODEL_SCRIPTMOUSECOMMAND_H

#include "v8datamodel/Mouse.h"
#include "v8datamodel/MouseCommand.h"

#include <memory>

namespace RBX {

class ScriptMouseCommand : public MouseCommand
{
private:
	std::shared_ptr<Mouse> mouse;

public:
	ScriptMouseCommand(Workspace* workspace);
	~ScriptMouseCommand() override;

	std::shared_ptr<Mouse> getMouse() { return mouse; }

	TextureId getCursorId() const override;
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseHover(const UIEvent& uiEvent) override;
	void onMouseIdle(const UIEvent& uiEvent) override;
	MouseCommand* onMouseUp(const UIEvent& uiEvent) override;
	MouseCommand* onPeekKeyDown(const UIEvent& uiEvent) override;
	const Name& getName() const override;
};

} // namespace RBX

#endif // V8DATAMODEL_SCRIPTMOUSECOMMAND_H
