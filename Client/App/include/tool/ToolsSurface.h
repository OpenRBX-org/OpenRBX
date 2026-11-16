#ifndef TOOL_TOOLSSURFACE_H
#define TOOL_TOOLSSURFACE_H

#include "tool/ToolsArrow.h"
#include "util/Name.h"
#include "util/NormalId.h"
#include "util/PV.h"
#include "util/SoundWorld.h"
#include "v8datamodel/MouseCommand.h"

#include <G3D/Vector2int16.h>
#include <G3D/Vector3.h>
#include <memory>
#include <string>

namespace RBX {

class Adorn;
class Instance;
class PartInstance;
class PVInstance;
class Surface;
class Workspace;
class XmlState;

extern char sFlatTool[];
extern char sGlueTool[];
extern char sWeldTool[];
extern char sStudsTool[];
extern char sInletTool[];
extern char sHingeTool[];
extern char sRightMotorTool[];
extern char sLeftMotorTool[];
extern char sOscillateMotorTool[];
extern char sModelSetFrontTool[];
extern char sAnchorTool[];
extern char sLockTool[];
extern char sResizeTool[];
extern char sSlingshotTool[];
extern char sRocketTool[];
extern char sHammerTool[];
extern char sLaserTool[];
extern char sCloneTool[];
extern char sGameTool[];

class SurfaceTool : public MouseCommand
{
protected:
	std::shared_ptr<PartInstance> part;
	Surface* surface;
	int surfaceId;
	std::string m_unk0x30;

	void render3dAdorn(Adorn* adorn) override;
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseHover(const UIEvent& uiEvent) override;

	virtual void VTable0x40(Surface* surface) = 0;

public:
	SurfaceTool(Workspace* workspace);
};

class FlatTool : public Named<SurfaceTool, sFlatTool>
{
private:
	void VTable0x40(Surface* surface) override;

public:
	FlatTool(Workspace* workspace) : Named<SurfaceTool, sFlatTool>(workspace) {}

	MouseCommand* isSticky() const override { return new FlatTool(workspace); }

	const std::string getCursorName() const override { return "FlatCursor"; }
};

class GlueTool : public Named<SurfaceTool, sGlueTool>
{
private:
	void VTable0x40(Surface* surface) override;

public:
	GlueTool(Workspace* workspace) : Named<SurfaceTool, sGlueTool>(workspace) {}

	MouseCommand* isSticky() const override { return new GlueTool(workspace); }

	const std::string getCursorName() const override { return "GlueCursor"; }
};

class WeldTool : public Named<SurfaceTool, sWeldTool>
{
private:
	void VTable0x40(Surface* surface) override;

public:
	WeldTool(Workspace* workspace) : Named<SurfaceTool, sWeldTool>(workspace) {}

	MouseCommand* isSticky() const override { return new WeldTool(workspace); }

	const std::string getCursorName() const override { return "WeldCursor"; }
};

class StudsTool : public Named<SurfaceTool, sStudsTool>
{
private:
	void VTable0x40(Surface* surface) override;

public:
	StudsTool(Workspace* workspace) : Named<SurfaceTool, sStudsTool>(workspace) {}

	MouseCommand* isSticky() const override { return new StudsTool(workspace); }

	const std::string getCursorName() const override { return "StudsCursor"; }
};

class InletTool : public Named<SurfaceTool, sInletTool>
{
private:
	void VTable0x40(Surface* surface) override;

public:
	InletTool(Workspace* workspace) : Named<SurfaceTool, sInletTool>(workspace) {}

	MouseCommand* isSticky() const override { return new InletTool(workspace); }

	const std::string getCursorName() const override { return "InletCursor"; }
};

class HingeTool : public Named<SurfaceTool, sHingeTool>
{
private:
	void VTable0x40(Surface* surface) override;

public:
	HingeTool(Workspace* workspace) : Named<SurfaceTool, sHingeTool>(workspace) {}

	MouseCommand* isSticky() const override { return new HingeTool(workspace); }

	const std::string getCursorName() const override { return "HingeCursor"; }
};

class RightMotorTool : public Named<SurfaceTool, sRightMotorTool>
{
private:
	void VTable0x40(Surface* surface) override;

public:
	RightMotorTool(Workspace* workspace) : Named<SurfaceTool, sRightMotorTool>(workspace) {}

	const std::string getCursorName() const override { return "MotorCursor"; }
};

class LeftMotorTool : public Named<SurfaceTool, sLeftMotorTool>
{
private:
	void VTable0x40(Surface* surface) override;

public:
	LeftMotorTool(Workspace* workspace) : Named<SurfaceTool, sLeftMotorTool>(workspace) {}

	const std::string getCursorName() const override { return "MotorCursor"; }
};

class OscillateMotorTool : public Named<SurfaceTool, sOscillateMotorTool>
{
private:
	void VTable0x40(Surface* surface) override;

public:
	OscillateMotorTool(Workspace* workspace) : Named<SurfaceTool, sOscillateMotorTool>(workspace) {}

	const std::string getCursorName() const override { return "MotorCursor"; }
};

class ModelSetFrontTool : public Named<SurfaceTool, sModelSetFrontTool>
{
private:
	void VTable0x40(Surface* surface) override;

public:
	ModelSetFrontTool(Workspace* workspace) : Named<SurfaceTool, sModelSetFrontTool>(workspace) {}

	const std::string getCursorName() const override { return "DirectionCursor"; }
};

class ModelTool : public MouseCommand
{
protected:
	std::shared_ptr<Instance> m_unk0x20;

	void render3dAdorn(Adorn* adorn) override;
	void onMouseHover(const UIEvent& uiEvent) override;

public:
	ModelTool(Workspace* workspace);
};

class LockTool : public Named<ModelTool, sLockTool>
{
private:
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	MouseCommand* onMouseUp(const UIEvent& uiEvent) override { return this; }
	const std::string getCursorName() const override;

public:
	LockTool(Workspace* workspace) : Named<ModelTool, sLockTool>(workspace) {}
};

class AnchorTool : public Named<ModelTool, sAnchorTool>
{
private:
	bool m_unk0x28;

	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	MouseCommand* onMouseUp(const UIEvent& uiEvent) override { return this; }
	void onMouseHover(const UIEvent& uiEvent) override;
	const std::string getCursorName() const override;

public:
	AnchorTool(Workspace* workspace) : Named<ModelTool, sAnchorTool>(workspace), m_unk0x28(false) {}
};

class ResizeTool : public Named<ArrowToolBase, sResizeTool>
{
private:
	std::weak_ptr<PVInstance> m_unk0x24;
	bool m_unk0x2c;
	NormalId m_unk0x30;
	G3D::Vector3 m_unk0x34;
	G3D::Vector2int16 m_unk0x40;
	int m_unk0x44;
	int m_unk0x48;

	void FUN_0063af80(int p_unk0x04);
	void FUN_0063b240(const UIEvent& uiEvent);

	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseHover(const UIEvent& uiEvent) override;
	void onMouseMove(const UIEvent& uiEvent) override;
	MouseCommand* onMouseUp(const UIEvent& uiEvent) override;
	const std::string getCursorName() const override;

public:
	ResizeTool(Workspace* workspace)
		: Named<ArrowToolBase, sResizeTool>(workspace), m_unk0x2c(false), m_unk0x44(0), m_unk0x48(0)
	{
	}

	MouseCommand* isSticky() const override { return new ResizeTool(workspace); }

	virtual void VTable0x40(XmlState* state);

	void render2d(Adorn* adorn) override;
	void render3dAdorn(Adorn* adorn) override;
};

class ShootTool : public MouseCommand
{
protected:
	std::shared_ptr<PartInstance> m_unk0x20;
	bool m_unk0x28;
	int m_unk0x2c;
	int m_unk0x30;

	void FUN_0063ab10(const UIEvent& uiEvent);

	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseIdle(const UIEvent& uiEvent) override;
	void onMouseMove(const UIEvent& uiEvent) override;

	MouseCommand* onMouseUp(const UIEvent& uiEvent) override
	{
		releaseCapture();
		return this;
	}

	const std::string getCursorName() const override;

public:
	ShootTool(Workspace* workspace, bool p_unk0x28, int p_unk0x2c);

	virtual std::shared_ptr<PartInstance> VTable0x40() = 0;
	virtual PV VTable0x44(const UIEvent& uiEvent, const CoordinateFrame& cframe) = 0;
	virtual SoundType VTable0x48() = 0;
};

class SlingshotTool : public Named<ShootTool, sSlingshotTool>
{
private:
	std::shared_ptr<PartInstance> VTable0x40() override;
	PV VTable0x44(const UIEvent& uiEvent, const CoordinateFrame& cframe) override;
	SoundType VTable0x48() override { return RUBBERBAND_SOUND; }

public:
	SlingshotTool(Workspace* workspace) : Named<ShootTool, sSlingshotTool>(workspace, true, 5) {}

	MouseCommand* isSticky() const override { return new SlingshotTool(workspace); }
};

class RocketTool : public Named<ShootTool, sRocketTool>
{
private:
	std::shared_ptr<PartInstance> VTable0x40() override;
	PV VTable0x44(const UIEvent& uiEvent, const CoordinateFrame& cframe) override;
	SoundType VTable0x48() override { return PING_SOUND; }

public:
	RocketTool(Workspace* workspace) : Named<ShootTool, sRocketTool>(workspace, false, 90) {}

	MouseCommand* isSticky() const override { return new RocketTool(workspace); }
};

class HammerTool : public Named<MouseCommand, sHammerTool>
{
private:
	std::shared_ptr<PartInstance> part;

	void render3dAdorn(Adorn* adorn) override;
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseIdle(const UIEvent& uiEvent) override;
	const std::string getCursorName() const override;

public:
	HammerTool(Workspace* workspace) : Named<MouseCommand, sHammerTool>(workspace) {}

	MouseCommand* isSticky() const override { return new HammerTool(workspace); }
};

class LaserTool : public Named<MouseCommand, sLaserTool>
{
private:
	int m_unk0x20;
	G3D::Vector3 m_unk0x24;
	G3D::Vector3 m_unk0x30;

	void render3dAdorn(Adorn* adorn) override;
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;

public:
	LaserTool(Workspace* workspace) : Named<MouseCommand, sLaserTool>(workspace), m_unk0x20(0) {}

	const std::string getCursorName() const override { return "GunCursor"; }

	MouseCommand* isSticky() const override { return new LaserTool(workspace); }
};

class CloneTool : public Named<MouseCommand, sCloneTool>
{
private:
	std::shared_ptr<PartInstance> part;

	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;
	void onMouseIdle(const UIEvent& uiEvent) override;
	const std::string getCursorName() const override;

public:
	CloneTool(Workspace* workspace);

	MouseCommand* isSticky() const override { return new CloneTool(workspace); }
};

class GameTool : public Named<MouseCommand, sGameTool>
{
private:
	std::string cursor;

	bool FUN_0063c2c0(PartInstance* part, const G3D::Vector3& hitWorld) const;

	void onMouseIdle(const UIEvent& uiEvent) override { onMouseHover(uiEvent); }
	void onMouseHover(const UIEvent& uiEvent) override;
	MouseCommand* onMouseDown(const UIEvent& uiEvent) override;

	const std::string getCursorName() const override { return cursor; }

public:
	GameTool(Workspace* workspace);
	~GameTool() override;

	MouseCommand* isSticky() const override { return new GameTool(workspace); }
};

} // namespace RBX

#endif // TOOL_TOOLSSURFACE_H
