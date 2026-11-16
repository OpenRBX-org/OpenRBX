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
	virtual ~ScriptMouseCommand();

	std::shared_ptr<Mouse> getMouse() { return mouse; }

	virtual TextureId getCursorId() const;
	virtual MouseCommand* onMouseDown(const UIEvent& uiEvent);
	virtual void onMouseHover(const UIEvent& uiEvent);
	virtual void onMouseIdle(const UIEvent& uiEvent);
	virtual MouseCommand* onMouseUp(const UIEvent& uiEvent);
	virtual MouseCommand* onPeekKeyDown(const UIEvent& uiEvent);
	virtual const Name& getName() const;
};

} // namespace RBX

#endif // V8DATAMODEL_SCRIPTMOUSECOMMAND_H
