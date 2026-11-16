#ifndef V8DATAMODEL_TOOLSPART_H
#define V8DATAMODEL_TOOLSPART_H

#include "util/Events.h"
#include "util/Name.h"
#include "v8datamodel/BrickColor.h"
#include "v8datamodel/MouseCommand.h"

#include <memory>
#include <string>

namespace RBX {

class Adorn;
class PartInstance;

extern char sFillTool[];
extern char sDropperTool[];
extern char sModelSetPrimaryPartTool[];

class PartTool : public MouseCommand
{
private:
	std::shared_ptr<PartInstance> partInstance;

protected:
	void render3dAdorn(Adorn* adorn) override;
	void onMouseHover(const UIEvent& uiEvent) override;

public:
	PartTool(Workspace* workspace);
};

class FillToolColor : public Notifier<FillToolColor, BrickColor>
{
private:
	BrickColor color;

public:
	FillToolColor() {}

	BrickColor get() const { return color; }

	void set(const BrickColor& value)
	{
		if (color != value) {
			color = value;
			raise(color);
		}
	}
};

class FillTool : public Named<PartTool, sFillTool>
{
public:
	static FillToolColor color;

protected:
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;

	const std::string getCursorName() const override { return "FillCursor"; }

public:
	FillTool(Workspace* workspace) : Named<PartTool, sFillTool>(workspace) {}

	MouseCommand* isSticky() const override { return new FillTool(workspace); }
};

class DropperTool : public Named<PartTool, sDropperTool>
{
protected:
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;

	const std::string getCursorName() const override { return "DropperCursor"; }

public:
	DropperTool(Workspace* workspace) : Named<PartTool, sDropperTool>(workspace) {}
};

class ModelSetPrimaryPartTool : public Named<PartTool, sModelSetPrimaryPartTool>
{
protected:
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;

public:
	ModelSetPrimaryPartTool(Workspace* workspace) : Named<PartTool, sModelSetPrimaryPartTool>(workspace) {}
};

} // namespace RBX

#endif // V8DATAMODEL_TOOLSPART_H
