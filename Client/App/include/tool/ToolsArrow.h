#ifndef TOOL_TOOLSARROW_H
#define TOOL_TOOLSARROW_H

#include "util/Extents.h"
#include "util/Name.h"
#include "util/NormalId.h"
#include "v8datamodel/MouseCommand.h"
#include "v8datamodel/Selection.h"
#include "v8tree/Service.h"
#include "v8tree/Verb.h"

#include <G3D/Ray.h>
#include <G3D/Rect2D.h>
#include <G3D/Vector2int16.h>
#include <G3D/Vector3.h>
#include <memory>
#include <set>
#include <string>

namespace RBX {

class Workspace;

extern char sBoxSelectCommand[];
class BoxSelectCommand : public Named<MouseCommand, sBoxSelectCommand>
{
private:
	ServiceClient<Selection> selection;
	bool reverseSelecting;
	G3D::Vector2int16 mouseDownView;
	G3D::Vector2int16 mouseCurrentView;
	std::set<Instance*> previousItemsInBox;

	void getMouseInstances(std::set<Instance*>& instances, const UIEvent& uiEvent, G3D::Rect2D& selectBox);
	void selectAnd(const std::set<Instance*>& newItemsInBox);
	void selectReverse(const std::set<Instance*>& newItemsInBox);

public:
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseMove(const UIEvent& uiEvent) override;
	void render2d(Adorn* adorn) override;

	BoxSelectCommand(Workspace* workspace);
};

extern char sArrowTool[];

class ArrowToolBase : public MouseCommand
{
protected:
	bool overInstance;

	void onMouseIdle(const UIEvent& uiEvent) override;
	void onMouseHover(const UIEvent& uiEvent) override;
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	const std::string getCursorName() const override;

public:
	ArrowToolBase(Workspace* workspace) : MouseCommand(workspace), overInstance(false) {}
};

class ArrowTool : public Named<ArrowToolBase, sArrowTool>
{
public:
	ArrowTool(Workspace* workspace) : Named<ArrowToolBase, sArrowTool>(workspace) {}

	MouseCommand* isSticky() const override { return new ArrowTool(workspace); }
};

class MegaDragger;

extern char sAxisMoveTool[];

class AxisMoveTool : public Named<ArrowToolBase, sAxisMoveTool>
{
private:
	std::unique_ptr<MegaDragger> dragger;
	std::string cursor;
	bool m_dragging;
	G3D::Vector2int16 m_mouseDownPosition;
	G3D::Ray m_handleRay;
	int m_axis;
	G3D::Vector3 m_position;

	static const G3D::Vector3& snapGrid()
	{
		static G3D::Vector3 v(1.0f, 0.4f, 1.0f);

		return v;
	}

	bool computeSelectionExtents(Extents& extents);
	bool hitTestSelectionHandles(const UIEvent& uiEvent, G3D::Vector3& hitPoint, NormalId& normalId);

	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseIdle(const UIEvent& uiEvent) override;
	void onMouseMove(const UIEvent& uiEvent) override;
	MouseCommand* onMouseUp(const UIEvent& uiEvent) override;
	const std::string getCursorName() const override { return cursor; }

public:
	AxisMoveTool(Workspace* workspace);

	MouseCommand* isSticky() const override { return new AxisMoveTool(workspace); }

	void render2d(Adorn* adorn) override;
	void render3dAdorn(Adorn* adorn) override;
};

class DataModel;

class UndoVerb : public Verb
{
public:
	UndoVerb(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;

private:
	DataModel* m_dataModel;
};

class RedoVerb : public Verb
{
public:
	RedoVerb(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;

private:
	DataModel* m_dataModel;
};

} // namespace RBX

#endif // TOOL_TOOLSARROW_H
