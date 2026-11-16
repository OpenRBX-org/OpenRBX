#ifndef V8DATAMODEL_COMMANDS_H
#define V8DATAMODEL_COMMANDS_H

#include "v8datamodel/MouseCommand.h"
#include "v8datamodel/Workspace.h"
#include "v8tree/Service.h"
#include "v8tree/Verb.h"
#include "v8world/Controller.h"

#include <memory>
#include <string>
#include <typeinfo>

class XmlElement;

namespace RBX {

class Camera;
class DataModel;
class RunService;
class Selection;

void FUN_00597490(XmlElement* container, std::shared_ptr<Instance> instance);

class FirstPersonCommand : public Verb
{
private:
	DataModel* dataModel;

public:
	FirstPersonCommand(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class StatsCommand : public Verb
{
private:
	DataModel* dataModel;

public:
	StatsCommand(DataModel* dataModel);

	bool isEnabled() const override;
	bool isChecked() const override;
	void doIt(IDataState* dataState) override;
};

class ForceAssertionCommand : public Verb
{
public:
	ForceAssertionCommand(DataModel* dataModel);

	void doIt(IDataState* dataState) override;
};

class EngineStatsCommand : public Verb
{
private:
	DataModel* dataModel;

public:
	EngineStatsCommand(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class JoinCommand : public Verb
{
private:
	DataModel* dataModel;

public:
	JoinCommand(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class ClearStarterpack : public Verb
{
private:
	DataModel* dataModel;

public:
	ClearStarterpack(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class ClearBackpack : public Verb
{
private:
	DataModel* dataModel;

public:
	ClearBackpack(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class RunStateVerb : public Verb
{
protected:
	DataModel* dataModel;
	ServiceClient<RunService> runService;

	RunStateVerb(std::string name, DataModel* dataModel);

	void playActionSound();

public:
	~RunStateVerb() override;
};

class SelectAllCommand : public RunStateVerb
{
public:
	SelectAllCommand(DataModel* dataModel) : RunStateVerb("SelectAll", dataModel) {}

	void doIt(IDataState* dataState) override;
};

class AllCanSelectCommand : public RunStateVerb
{
public:
	AllCanSelectCommand(DataModel* dataModel) : RunStateVerb("AllCanSelect", dataModel) {}

	void doIt(IDataState* dataState) override;
};

class RunCommand : public RunStateVerb
{
public:
	RunCommand(DataModel* dataModel) : RunStateVerb("Run", dataModel) {}

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class StopCommand : public RunStateVerb
{
public:
	StopCommand(DataModel* dataModel) : RunStateVerb("Stop", dataModel) {}

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class ResetCommand : public RunStateVerb
{
public:
	ResetCommand(DataModel* dataModel) : RunStateVerb("Reset", dataModel) {}

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

template <class T, class Base>
class TToolVerb : public Base
{
public:
	TToolVerb(DataModel* dataModel) : Base(T::name().toString() + "Tool", dataModel) {}

	bool isChecked() const override
	{
		MouseCommand* command = this->dataModel->getWorkspace()->getCurrentMouseCommand();
		if (command) {
			return typeid(T) == typeid(*command);
		}
		return false;
	}

	void doIt(IDataState* dataState) override { this->dataModel->getWorkspace()->setMouseCommand(VTable0x14()); }

	virtual MouseCommand* VTable0x14() { return new T(this->dataModel->getWorkspace()); }
};

class EditSelectionVerb : public Verb
{
protected:
	std::shared_ptr<Workspace> workspace;
	ServiceClient<Selection> selection;
	DataModel* dataModel;

	EditSelectionVerb(VerbContainer* container, std::string name, DataModel* dataModel);
	EditSelectionVerb(std::string name, DataModel* dataModel);

public:
	~EditSelectionVerb() override;

	bool isEnabled() const override;
};

class DeleteBase : public EditSelectionVerb
{
protected:
	bool m_unk0x24;

	DeleteBase(DataModel* dataModel, std::string name, bool p_unk0x24);
	DeleteBase(VerbContainer* container, DataModel* dataModel, std::string name);

public:
	void doIt(IDataState* dataState) override;
};

class PlayDeleteSelectionVerb : public DeleteBase
{
public:
	PlayDeleteSelectionVerb(DataModel* dataModel) : DeleteBase(dataModel, "PlayDelete", true) {}
};

class DeleteSelectionVerb : public DeleteBase
{
public:
	DeleteSelectionVerb(DataModel* dataModel) : DeleteBase(dataModel, "Delete", false) {}

	DeleteSelectionVerb(VerbContainer* container, DataModel* dataModel, std::string name)
		: DeleteBase(container, dataModel, name)
	{
	}
};

class CanNotSelectCommand : public EditSelectionVerb
{
public:
	CanNotSelectCommand(DataModel* dataModel) : EditSelectionVerb("CanNotSelect", dataModel) {}

	void doIt(IDataState* dataState) override;
};

class RotateAxisCommand : public EditSelectionVerb
{
protected:
	RotateAxisCommand(std::string name, DataModel* dataModel);

	virtual G3D::Matrix3 getRotationAxis() = 0;

private:
	void FUN_00598da0(const G3D::Matrix3& rotation, const std::vector<PVInstance*>& pvInstances);

public:
	void doIt(IDataState* dataState) override;
};

class ControllerCommand : public EditSelectionVerb
{
protected:
	ControllerCommand(std::string name, DataModel* dataModel);

	void setController(Controller::ControllerType controllerType);

public:
	bool isEnabled() const override;
};

class BoolPropertyVerb : public EditSelectionVerb
{
private:
	const Name& propertyName;

public:
	BoolPropertyVerb(const std::string& name, DataModel* dataModel, const char* property);

protected:
	void doIt(IDataState* dataState) override;
	bool isChecked() const override;
};

class AnchorVerb : public BoolPropertyVerb
{
public:
	AnchorVerb(DataModel* dataModel) : BoolPropertyVerb("AnchorVerb", dataModel, "Anchored") {}
};

class TranslucentVerb : public BoolPropertyVerb
{
public:
	TranslucentVerb(DataModel* dataModel) : BoolPropertyVerb("TranslucentVerb", dataModel, "Transparent") {}
};

class CanCollideVerb : public BoolPropertyVerb
{
public:
	CanCollideVerb(DataModel* dataModel) : BoolPropertyVerb("CanCollideVerb", dataModel, "CanCollide") {}
};

class MoveUpSelectionVerb : public EditSelectionVerb
{
private:
	float moveUpHeight;

public:
	MoveUpSelectionVerb(DataModel* dataModel, const std::string& name, float moveUpHeight)
		: EditSelectionVerb(name, dataModel), moveUpHeight(moveUpHeight)
	{
	}

	void doIt(IDataState* dataState) override;
};

class MoveUpPlateVerb : public MoveUpSelectionVerb
{
public:
	MoveUpPlateVerb(DataModel* dataModel) : MoveUpSelectionVerb(dataModel, "SelectionUpPlate", 0.4f) {}
};

class MoveUpBrickVerb : public MoveUpSelectionVerb
{
public:
	MoveUpBrickVerb(DataModel* dataModel) : MoveUpSelectionVerb(dataModel, "SelectionUpBrick", 1.2f) {}
};

class RotateSelectionVerb : public RotateAxisCommand
{
public:
	RotateSelectionVerb(DataModel* dataModel);

protected:
	G3D::Matrix3 getRotationAxis() override;
};

class TiltSelectionVerb : public RotateAxisCommand
{
public:
	TiltSelectionVerb(DataModel* dataModel);

protected:
	G3D::Matrix3 getRotationAxis() override;
};

class PrimaryControllerCommand : public ControllerCommand
{
public:
	PrimaryControllerCommand(DataModel* dataModel);

	void doIt(IDataState* dataState) override;
};

class SecondaryControllerCommand : public ControllerCommand
{
public:
	SecondaryControllerCommand(DataModel* dataModel);

	void doIt(IDataState* dataState) override;
};

class GroupSelectionVerb : public EditSelectionVerb
{
public:
	GroupSelectionVerb(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class WeldSelectionVerb : public EditSelectionVerb
{
public:
	WeldSelectionVerb(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class SnapSelectionVerb : public EditSelectionVerb
{
public:
	SnapSelectionVerb(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class UngroupSelectionVerb : public EditSelectionVerb
{
public:
	UngroupSelectionVerb(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class DuplicateSelectionVerb : public EditSelectionVerb
{
public:
	DuplicateSelectionVerb(DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class AttachCameraCommand : public EditSelectionVerb
{
public:
	AttachCameraCommand(DataModel* dataModel);

	bool isEnabled() const override;
	bool isChecked() const override;
	void doIt(IDataState* dataState) override;
};

class TrackCameraCommand : public EditSelectionVerb
{
public:
	TrackCameraCommand(DataModel* dataModel);

	bool isEnabled() const override;
	bool isChecked() const override;
	void doIt(IDataState* dataState) override;
};

class FollowCameraCommand : public EditSelectionVerb
{
public:
	FollowCameraCommand(DataModel* dataModel);

	bool isEnabled() const override;
	bool isChecked() const override;
	void doIt(IDataState* dataState) override;
};

class WatchCameraCommand : public EditSelectionVerb
{
public:
	WatchCameraCommand(DataModel* dataModel);

	bool isEnabled() const override;
	bool isChecked() const override;
	void doIt(IDataState* dataState) override;
};

class MoveDownSelectionVerb : public EditSelectionVerb
{
public:
	MoveDownSelectionVerb(DataModel* dataModel);

	void doIt(IDataState* dataState) override;
};

class CameraVerb : public Verb
{
protected:
	Workspace* workspace;

	Camera* getCamera() const { return workspace->getCamera(); }

	ServiceClient<Selection> selection;

public:
	CameraVerb(std::string name, Workspace* workspace);

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class CameraPanLeftCommand : public CameraVerb
{
public:
	CameraPanLeftCommand(Workspace* workspace) : CameraVerb("CameraPanLeft", workspace) {}

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class CameraPanRightCommand : public CameraVerb
{
public:
	CameraPanRightCommand(Workspace* workspace) : CameraVerb("CameraPanRight", workspace) {}

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class CameraTiltUpCommand : public CameraVerb
{
public:
	CameraTiltUpCommand(Workspace* workspace) : CameraVerb("CameraTiltUp", workspace) {}

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class CameraTiltDownCommand : public CameraVerb
{
public:
	CameraTiltDownCommand(Workspace* workspace) : CameraVerb("CameraTiltDown", workspace) {}

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class CameraZoomInCommand : public CameraVerb
{
public:
	CameraZoomInCommand(Workspace* workspace) : CameraVerb("CameraZoomIn", workspace) {}

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class CameraZoomOutCommand : public CameraVerb
{
public:
	CameraZoomOutCommand(Workspace* workspace) : CameraVerb("CameraZoomOut", workspace) {}

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class CameraModelViewCommand : public CameraVerb
{
public:
	CameraModelViewCommand(Workspace* workspace) : CameraVerb("CameraModelView", workspace) {}

	void doIt(IDataState* dataState) override;
};

class FixedCameraCommand : public CameraVerb
{
public:
	FixedCameraCommand(Workspace* workspace) : CameraVerb("FixedCamera", workspace) {}

	bool isEnabled() const override;
	bool isChecked() const override;
	void doIt(IDataState* dataState) override;
};

class CameraCenterCommand : public CameraVerb
{
public:
	CameraCenterCommand(Workspace* workspace) : CameraVerb("CameraCenter", workspace) {}

	bool isEnabled() const override;
	void doIt(IDataState* dataState) override;
};

class CameraZoomExtentsCommand : public CameraVerb
{
public:
	CameraZoomExtentsCommand(Workspace* workspace) : CameraVerb("CameraZoomExtents", workspace) {}

	void doIt(IDataState* dataState) override;
};

} // namespace RBX

#endif // V8DATAMODEL_COMMANDS_H
