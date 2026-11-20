#include "v8datamodel/Commands.h"

#include "gui/GUI.h"
#include "network/Player.h"
#include "network/Players.h"
#include "tool/GrabTool.h"
#include "tool/MegaDragger.h"
#include "tool/NullTool.h"
#include "tool/ToolsArrow.h"
#include "tool/ToolsSurface.h"
#include "util/Math.h"
#include "util/Sound.h"
#include "v8datamodel/Backpack.h"
#include "v8datamodel/Camera.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Feature.h"
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/ToolsPart.h"
#include "v8world/SpatialHash.h"
#include "v8world/World.h"
#include "v8xml/SerializerV2.h"

#include <algorithm>
#include <memory>

template class RBX::TToolVerb<RBX::ArrowTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::ModelSetPrimaryPartTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::FillTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::DropperTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::HammerTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::LaserTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::FlatTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::GlueTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::WeldTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::StudsTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::InletTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::HingeTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::RightMotorTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::LeftMotorTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::OscillateMotorTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::ModelSetFrontTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::AnchorTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::LockTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::ResizeTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::SlingshotTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::RocketTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::NullTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::GrabTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::CloneTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::GameTool, RBX::RunStateVerb>;
template class RBX::TToolVerb<RBX::AxisMoveTool, RBX::RunStateVerb>;

namespace RBX {

static void checkEditPermission(DataModel* dataModel)
{
	if (Network::Players::clientIsPresent(dataModel, true) && !Network::Players::serverIsPresent(dataModel, true)) {
		throw std::runtime_error("Insufficinet permissions");
	}
}

inline void RunStateVerb::playActionSound()
{
	checkEditPermission(dataModel);
}

FirstPersonCommand::FirstPersonCommand(DataModel* dataModel)
	: Verb(dataModel, "FirstPersonIndicator"), dataModel(dataModel)
{
}

StatsCommand::StatsCommand(DataModel* dataModel) : Verb(dataModel, "Stats"), dataModel(dataModel)
{
}

ForceAssertionCommand::ForceAssertionCommand(DataModel* dataModel) : Verb(dataModel, "ForceAssertion")
{
}

EngineStatsCommand::EngineStatsCommand(DataModel* dataModel) : Verb(dataModel, "EngineStats"), dataModel(dataModel)
{
}

JoinCommand::JoinCommand(DataModel* dataModel) : Verb(dataModel, "JoinCommand"), dataModel(dataModel)
{
}

ClearStarterpack::ClearStarterpack(DataModel* dataModel) : Verb(dataModel, "ClearStarterpack"), dataModel(dataModel)
{
}

ClearBackpack::ClearBackpack(DataModel* dataModel) : Verb(dataModel, "ClearBackpack"), dataModel(dataModel)
{
}

void ClearBackpack::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	if (Network::Player* player = Network::Players::findLocalPlayer(dataModel)) {
		player->getPlayerBackpack()->removeAllChildren();
	}
}

G3D::Matrix3 RotateSelectionVerb::getRotationAxis()
{
	return Math::matrixRotateY();
}

static void applyControllerType(Controller::ControllerType controllerType, PVInstance* instance)
{
	instance->setControllerType(controllerType);
}

static bool isControllable(PVInstance* instance)
{
	return instance->isControllable();
}

bool CameraTiltUpCommand::isEnabled() const
{
	return getCamera()->canTilt(-1);
}

bool CameraTiltDownCommand::isEnabled() const
{
	return getCamera()->canTilt(1);
}

bool CameraZoomInCommand::isEnabled() const
{
	return getCamera()->canZoom(1);
}

bool CameraZoomOutCommand::isEnabled() const
{
	return getCamera()->canZoom(-1);
}

bool FixedCameraCommand::isChecked() const
{
	return getCamera()->getCameraType() == Camera::FIXED_CAMERA;
}

void SelectAllCommand::doIt(IDataState* dataState)
{
	playActionSound();
	dataModel->getWorkspace()->selectAll();
}

static void lockInstance(std::shared_ptr<Instance> instance)
{
	PartInstance::setLocked(instance.get(), true);
}

bool FirstPersonCommand::isEnabled() const
{
	return dataModel->getWorkspace()->getCamera()->isFirstPersonCamera();
}

void StatsCommand::doIt(IDataState* dataState)
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();

	if (TopMenuBar* bar = dynamic_cast<TopMenuBar*>(guiRoot->findFirstChildByName("StatsHud1"))) {
		bar->setVisible(!bar->isVisible());
	}

	if (TopMenuBar* bar = dynamic_cast<TopMenuBar*>(guiRoot->findFirstChildByName("StatsHud2"))) {
		bar->setVisible(!bar->isVisible());
	}
}

bool StatsCommand::isEnabled() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	return dynamic_cast<TopMenuBar*>(guiRoot->findFirstChildByName("StatsHud1")) != nullptr;
}

bool StatsCommand::isChecked() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	TopMenuBar* bar = dynamic_cast<TopMenuBar*>(guiRoot->findFirstChildByName("StatsHud1"));
	return bar && bar->isVisible();
}

void EngineStatsCommand::doIt(IDataState* dataState)
{
	dataModel->getWorkspace()->getWorld()->getContactManager().getSpatialHash().doStats();
}

static void recordDelete(UndoState* undoState, RedoState* redoState, std::shared_ptr<Instance> instance)
{
	undoState->addParentProperty(*instance);
	redoState->addDelete(*instance);
}

G3D::Matrix3 TiltSelectionVerb::getRotationAxis()
{
	return Math::matrixTiltQuadrant(Math::toYAxisQuadrant(workspace->getGCamera().getCoordinateFrame()));
}

bool AttachCameraCommand::isChecked() const
{
	return workspace->getCamera()->getCameraType() == Camera::ATTACH_CAMERA;
}

bool TrackCameraCommand::isChecked() const
{
	return workspace->getCamera()->getCameraType() == Camera::TRACK_CAMERA;
}

bool FollowCameraCommand::isChecked() const
{
	return workspace->getCamera()->getCameraType() == Camera::FOLLOW_CAMERA;
}

bool WatchCameraCommand::isChecked() const
{
	return workspace->getCamera()->getCameraType() == Camera::WATCH_CAMERA;
}

bool ClearBackpack::isEnabled() const
{
	if (Network::Player* player = Network::Players::findLocalPlayer(dataModel)) {
		if (Backpack* backpack = player->getPlayerBackpack()) {
			return backpack->numChildren() > 0;
		}
	}

	return false;
}

static bool hasChildren(ModelInstance* model)
{
	return model->numChildren() > 0;
}

void appendInstanceXml(XmlElement* container, std::shared_ptr<Instance> instance)
{
	XmlElement* child = instance->write();

	if (child != nullptr) {
		container->addChild(child);
	}
}

RunStateVerb::~RunStateVerb() = default;

EditSelectionVerb::~EditSelectionVerb() = default;

RunStateVerb::RunStateVerb(std::string name, DataModel* dataModel)
	: Verb(dataModel, name), dataModel(dataModel), runService(dataModel)
{
}

CameraVerb::CameraVerb(std::string name, Workspace* workspace)
	: Verb(workspace, name), workspace(workspace), selection(workspace)
{
}

bool CameraVerb::isEnabled() const
{
	return true;
}

static void weldsToStuds(std::shared_ptr<Instance> instance)
{
	if (PartInstance* part = dynamic_cast<PartInstance*>(instance.get())) {
		Surfaces& surfaces = part->getSurfaces();
		for (int i = 0; i < 6; i++) {
			if (surfaces[(NormalId) i].getSurfaceType() == WELD) {
				surfaces[(NormalId) i].setSurfaceType(STUDS);
			}
		}
	}

	if (instance->getChildren()) {
		std::for_each(instance->getChildren()->begin(), instance->getChildren()->end(), weldsToStuds);
	}
}

class SetBoolProperty
{
public:
	SetBoolProperty(const Name& name, bool value)
		: m_name(name), m_value(value), m_undoState(new UndoState()), m_redoState(new RedoState())
	{
	}

	void operator()(std::shared_ptr<Instance> instance);

	const Name& m_name;
	bool m_value;
	std::unique_ptr<UndoState> m_undoState;
	std::unique_ptr<RedoState> m_redoState;
};

EditSelectionVerb::EditSelectionVerb(std::string name, DataModel* dataModel)
	: Verb(dataModel, name), workspace(shared_from(dataModel->getWorkspace())), selection(dataModel),
	  dataModel(dataModel)
{
}

GroupSelectionVerb::GroupSelectionVerb(DataModel* dataModel) : EditSelectionVerb("Group", dataModel)
{
}

WeldSelectionVerb::WeldSelectionVerb(DataModel* dataModel) : EditSelectionVerb("WeldSelection", dataModel)
{
}

SnapSelectionVerb::SnapSelectionVerb(DataModel* dataModel) : EditSelectionVerb("SnapSelection", dataModel)
{
}

UngroupSelectionVerb::UngroupSelectionVerb(DataModel* dataModel) : EditSelectionVerb("UnGroup", dataModel)
{
}

DuplicateSelectionVerb::DuplicateSelectionVerb(DataModel* dataModel) : EditSelectionVerb("Duplicate", dataModel)
{
}

DeleteBase::DeleteBase(DataModel* dataModel, std::string name, bool playDelete)
	: EditSelectionVerb(name, dataModel), m_playDelete(playDelete)
{
}

AttachCameraCommand::AttachCameraCommand(DataModel* dataModel) : EditSelectionVerb("AttachCamera", dataModel)
{
}

TrackCameraCommand::TrackCameraCommand(DataModel* dataModel) : EditSelectionVerb("TrackCamera", dataModel)
{
}

FollowCameraCommand::FollowCameraCommand(DataModel* dataModel) : EditSelectionVerb("FollowCamera", dataModel)
{
}

WatchCameraCommand::WatchCameraCommand(DataModel* dataModel) : EditSelectionVerb("WatchCamera", dataModel)
{
}

MoveDownSelectionVerb::MoveDownSelectionVerb(DataModel* dataModel) : EditSelectionVerb("SelectionDown", dataModel)
{
}

bool JoinCommand::isEnabled() const
{
	Selection* currentSelection = dataModel ? dataModel->create<Selection>() : nullptr;

	if (currentSelection->size() == 2) {
		std::shared_ptr<Instance> first = currentSelection->front();
		std::shared_ptr<Instance> second = currentSelection->back();

		if (canJoinMotor(first.get(), second.get())) {
			return true;
		}
	}

	return false;
}

void JoinCommand::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	Selection* currentSelection = dataModel ? dataModel->create<Selection>() : nullptr;

	if (currentSelection->size() == 2) {
		std::shared_ptr<Instance> first = currentSelection->front();
		std::shared_ptr<Instance> second = currentSelection->back();
		joinMotor(first.get(), second.get());
	}
}

bool ClearStarterpack::isEnabled() const
{
	if (dataModel) {
		if (StarterPackService* starterPack = dataModel->find<StarterPackService>()) {
			return starterPack->numChildren() > 0;
		}
	}

	return false;
}

void ClearStarterpack::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	if (dataModel) {
		if (StarterPackService* starterPack = dataModel->find<StarterPackService>()) {
			starterPack->removeAllChildren();
		}
	}
}

static void gluesToWelds(std::shared_ptr<Instance> instance)
{
	if (PartInstance* part = dynamic_cast<PartInstance*>(instance.get())) {
		Surfaces& surfaces = part->getSurfaces();
		for (int i = 0; i < 6; i++) {
			if (surfaces[(NormalId) i].getSurfaceType() == GLUE) {
				surfaces[(NormalId) i].setSurfaceType(WELD);
			}
		}
	}

	if (instance->getChildren()) {
		std::for_each(instance->getChildren()->begin(), instance->getChildren()->end(), gluesToWelds);
	}
}

RotateAxisCommand::RotateAxisCommand(std::string name, DataModel* dataModel) : EditSelectionVerb(name, dataModel)
{
}

ControllerCommand::ControllerCommand(std::string name, DataModel* dataModel) : EditSelectionVerb(name, dataModel)
{
}

BoolPropertyVerb::BoolPropertyVerb(const std::string& name, DataModel* dataModel, const char* property)
	: EditSelectionVerb(name, dataModel), propertyName(Name::declare(property, -1))
{
}

static bool isBoolPropertySet(const Name& propertyName, std::shared_ptr<const Instance> instance)
{
	Reflection::MemberDescriptorContainer<Reflection::PropertyDescriptor>::ConstIterator iter =
		instance->findProperty(propertyName);
	if (iter != instance->properties_end()) {
		if (dynamic_cast<const Reflection::TypedPropertyDescriptor<bool>*>(&iter.getDescriptor())) {
			if ((*iter).getValue<bool>()) {
				return true;
			}
		}
	}

	return false;
}

void SetBoolProperty::operator()(std::shared_ptr<Instance> instance)
{
	Reflection::MemberDescriptorContainer<Reflection::PropertyDescriptor>::Iterator iter =
		instance->findProperty(m_name);

	if (iter != instance->properties_end()) {
		if (dynamic_cast<const Reflection::TypedPropertyDescriptor<bool>*>(&(*iter).getDescriptor())) {
			if ((*iter).getValue<bool>() != m_value) {
				m_undoState->addProperty(*iter);
				(*iter).setValue<bool>(m_value);
				m_redoState->addProperty(*iter);
			}
		}
	}
}

void RotateAxisCommand::rotate(const G3D::Matrix3& rotation, const std::vector<PVInstance*>& pvInstances)
{
	if (pvInstances.size() > 0) {
		MegaDragger dragger(nullptr, pvInstances, workspace.get());
		dragger.startDragging();
		dragger.safeRotate(rotation);
		dragger.finishDragging();

		workspace->raiseDrawChanged();
	}
}

RotateSelectionVerb::RotateSelectionVerb(DataModel* dataModel) : RotateAxisCommand("SelectionRotate", dataModel)
{
}

TiltSelectionVerb::TiltSelectionVerb(DataModel* dataModel) : RotateAxisCommand("SelectionTilt", dataModel)
{
}

PrimaryControllerCommand::PrimaryControllerCommand(DataModel* dataModel) : ControllerCommand("Controller1", dataModel)
{
}

SecondaryControllerCommand::SecondaryControllerCommand(DataModel* dataModel)
	: ControllerCommand("Controller2", dataModel)
{
}

class UngroupModel
{
public:
	UngroupModel(std::vector<Instance*>& newSelection, bool& changed) : m_newSelection(newSelection), m_changed(changed)
	{
	}

	void operator()(const std::shared_ptr<Instance>& instance)
	{
		if (instance->numChildren() > 0) {
			for (unsigned int i = 0; i < instance->numChildren(); i++) {
				m_newSelection.push_back(instance->getChild(i));
			}

			instance->promoteChildren();
			instance->setParent(nullptr);
			m_changed = true;
		}
	}

private:
	std::vector<Instance*>& m_newSelection;
	bool& m_changed;
};

EditSelectionVerb::EditSelectionVerb(VerbContainer* container, std::string name, DataModel* dataModel)
	: Verb(container, name), workspace(shared_from(dataModel ? dataModel->find<Workspace>() : nullptr)),
	  selection(dataModel), dataModel(dataModel)
{
}

bool DuplicateSelectionVerb::isEnabled() const
{
	return (dataModel ? dataModel->create<Selection>() : nullptr)->size() > 0;
}

DeleteBase::DeleteBase(VerbContainer* container, DataModel* dataModel, std::string name)
	: EditSelectionVerb(container, name, dataModel), m_playDelete(false)
{
}

class ControllerTypeDiffers
{
public:
	ControllerTypeDiffers(Controller::ControllerType controllerType) : m_controllerType(controllerType) {}

	bool operator()(PVInstance* instance) const { return instance->getControllerType() != m_controllerType; }

	Controller::ControllerType m_controllerType;
};

void ControllerCommand::setController(Controller::ControllerType controllerType)
{
	FilteredSelection<PVInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;

	Controller::ControllerType newType = filteredSelection->findFirst(ControllerTypeDiffers(controllerType))
											 ? controllerType
											 : Controller::NO_CONTROLLER;

	std::for_each(filteredSelection->begin(), filteredSelection->end(), [newType](PVInstance* instance) {
		applyControllerType(newType, instance);
	});
}

void AllCanSelectCommand::doIt(IDataState* dataState)
{
	playActionSound();
	PartInstance::setLocked(dataModel->getWorkspace(), false);
	dataState->clearStack();
	dataState->setDirty(true);
}

void GroupSelectionVerb::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	Selection* currentSelection = dataModel ? dataModel->create<Selection>() : nullptr;
	ModelInstance* model = workspace->createModel(currentSelection->begin(), currentSelection->end()).get();
	currentSelection->setSelection(model);

	(dataModel ? dataModel->create<Soundscape::SoundService>() : nullptr)->playSound(PING_SOUND);
	dataState->clearStack();
	dataState->setDirty(true);
}

void UngroupSelectionVerb::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	FilteredSelection<ModelInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<ModelInstance>>() : nullptr;

	std::vector<std::shared_ptr<Instance>> models;
	for (std::vector<ModelInstance*>::const_iterator iter = filteredSelection->begin();
		 iter != filteredSelection->end();
		 ++iter) {
		ModelInstance* model = *iter;
		models.push_back(model->shared_from_this());
	}

	std::vector<Instance*> newSelection;
	bool changed = false;
	std::for_each(models.begin(), models.end(), UngroupModel(newSelection, changed));

	if (changed) {
		filteredSelection->setSelection(newSelection.begin(), newSelection.end());
		(dataModel ? dataModel->create<Soundscape::SoundService>() : nullptr)->playSound(PING_SOUND);
	}

	dataState->clearStack();
	dataState->setDirty(true);
}

void RotateAxisCommand::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	FilteredSelection<PVInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;

	UndoState* undoState = new UndoState();

	for (std::vector<PVInstance*>::const_iterator iter = filteredSelection->items().begin();
		 iter != filteredSelection->items().end();
		 ++iter) {
		PVInstance* pvInstance = *iter;
	}

	rotate(getRotationAxis(), filteredSelection->items());

	(dataModel ? dataModel->create<Soundscape::SoundService>() : nullptr)->playSound(CLICK_SOUND);

	workspace->raiseDrawChanged();

	RedoState* redoState = new RedoState();

	for (std::vector<PVInstance*>::const_iterator iter = filteredSelection->items().begin();
		 iter != filteredSelection->items().end();
		 ++iter) {
		PVInstance* pvInstance = *iter;
	}

	dataState->pushState(getName(), undoState, redoState);
	dataState->setDirty(true);
}

void AttachCameraCommand::doIt(IDataState* dataState)
{
	FilteredSelection<PVInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;
	PVInstance* subject = filteredSelection->items().size() > 0 ? filteredSelection->items().front() : nullptr;

	workspace->getCamera()->setCameraType(Camera::ATTACH_CAMERA);
	workspace->getCamera()->setCameraSubject(subject);
	dataState->clearStack();
	dataState->setDirty(true);
}

void TrackCameraCommand::doIt(IDataState* dataState)
{
	Selection* currentSelection = dataModel ? dataModel->create<Selection>() : nullptr;

	if (PVInstance* subject = dynamic_cast<PVInstance*>(currentSelection->front().get())) {
		workspace->getCamera()->setCameraSubject(subject);
		workspace->getCamera()->setCameraType(Camera::TRACK_CAMERA);
	}

	dataState->clearStack();
	dataState->setDirty(true);
}

void FollowCameraCommand::doIt(IDataState* dataState)
{
	FilteredSelection<PVInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;
	PVInstance* subject = filteredSelection->items().size() > 0 ? filteredSelection->items().front() : nullptr;

	workspace->getCamera()->setCameraType(Camera::FOLLOW_CAMERA);
	workspace->getCamera()->setCameraSubject(subject);
	dataState->clearStack();
	dataState->setDirty(true);
}

void WatchCameraCommand::doIt(IDataState* dataState)
{
	FilteredSelection<PVInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;
	PVInstance* subject = filteredSelection->items().size() > 0 ? filteredSelection->items().front() : nullptr;

	workspace->getCamera()->setCameraType(Camera::WATCH_CAMERA);
	workspace->getCamera()->setCameraSubject(subject);
	dataState->clearStack();
	dataState->setDirty(true);
}

void MoveUpSelectionVerb::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	FilteredSelection<PVInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;

	if (filteredSelection->items().size() > 0) {
		MegaDragger dragger(nullptr, filteredSelection->items(), workspace.get());
		dragger.startDragging();
		dragger.safeMoveNoDrop(G3D::Vector3(0.0f, moveUpHeight, 0.0f));
		dragger.finishDragging();

		(dataModel ? dataModel->create<Soundscape::SoundService>() : nullptr)->playSound(CLICK_SOUND);

		workspace->raiseDrawChanged();
		dataState->clearStack();
		dataState->setDirty(true);
	}
}

void MoveDownSelectionVerb::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	FilteredSelection<PVInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;

	if (filteredSelection->items().size() > 0) {
		MegaDragger dragger(nullptr, filteredSelection->items(), workspace.get());
		dragger.startDragging();
		G3D::Vector3 moved = dragger.safeMoveNoDrop(G3D::Vector3(0.0f, -1.2f, 0.0f));
		dragger.finishDragging();

		SoundType sound = moved.y != 0.0f ? CLICK_SOUND : BOING_SOUND;
		(dataModel ? dataModel->create<Soundscape::SoundService>() : nullptr)->playSound(sound);

		workspace->raiseDrawChanged();
		dataState->clearStack();
		dataState->setDirty(true);
	}
}

void PrimaryControllerCommand::doIt(IDataState* dataState)
{
	setController(Controller::PRIMARY_CONTROLLER);
	dataState->clearStack();
	dataState->setDirty(true);
}

void SecondaryControllerCommand::doIt(IDataState* dataState)
{
	setController(Controller::SECONDARY_CONTROLLER);
	dataState->clearStack();
	dataState->setDirty(true);
}

void CameraVerb::doIt(IDataState* dataState)
{
	ServiceProvider::create<Soundscape::SoundService>(workspace)->playSound(STEP_SOUND);
	dataState->clearStack();
	dataState->setDirty(true);
}

void CameraPanLeftCommand::doIt(IDataState* dataState)
{
	getCamera()->panUnits(-1);
	CameraVerb::doIt(dataState);
}

void CameraPanRightCommand::doIt(IDataState* dataState)
{
	getCamera()->panUnits(1);
	CameraVerb::doIt(dataState);
}

void CameraTiltUpCommand::doIt(IDataState* dataState)
{
	if (getCamera()->tiltUnits(-1)) {
		CameraVerb::doIt(dataState);
	}
}

void CameraTiltDownCommand::doIt(IDataState* dataState)
{
	if (getCamera()->tiltUnits(1)) {
		CameraVerb::doIt(dataState);
	}
}

void CameraZoomInCommand::doIt(IDataState* dataState)
{
	if (getCamera()->zoom(1.0f)) {
		CameraVerb::doIt(dataState);
	}
}

void CameraZoomOutCommand::doIt(IDataState* dataState)
{
	if (getCamera()->zoom(-1.0f)) {
		CameraVerb::doIt(dataState);
	}
}

void CameraModelViewCommand::doIt(IDataState* dataState)
{
	workspace->setImageServerView(workspace->getViewPort());
	ServiceProvider::create<Soundscape::SoundService>(workspace)->playSound(STEP_SOUND);
	dataState->clearStack();
	dataState->setDirty(true);
}

void FixedCameraCommand::doIt(IDataState* dataState)
{
	getCamera()->setCameraType(Camera::FIXED_CAMERA);
	dataState->clearStack();
	dataState->setDirty(true);
}

bool BoolPropertyVerb::isChecked() const
{
	return std::find_if(selection->begin(), selection->end(), [this](const std::shared_ptr<Instance>& instance) {
			   return isBoolPropertySet(propertyName, instance);
		   }) != selection->end();
}

void BoolPropertyVerb::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	SetBoolProperty functor(propertyName, !isChecked());
	std::for_each(selection->begin(), selection->end(), std::move(functor));
	dataState->pushState(propertyName.toString(), functor.m_undoState.release(), functor.m_redoState.release());
	dataState->setDirty(true);
}

bool CameraCenterCommand::isEnabled() const
{
	bool hasSelection = ServiceClient<FilteredSelection<PVInstance>>(workspace)->items().size() >= 1;

	if (hasSelection) {
		Camera* camera = getCamera();

		if (camera && camera->getCameraType() == Camera::FIXED_CAMERA) {
			return true;
		}
	}

	return false;
}

void CameraCenterCommand::doIt(IDataState* dataState)
{
	ServiceClient<FilteredSelection<PVInstance>> filteredSelection(workspace);
	G3D::Vector3 center;

	if (filteredSelection->items().size() >= 1) {
		const std::vector<PVInstance*>& items = filteredSelection->items();
		Extents extents = items[0]->getExtentsWorld();

		for (unsigned int i = 1; i < items.size(); i++) {
			extents.unionWith(items[i]->getExtentsWorld());
		}

		center = extents.center();
	}
	else {
		center = G3D::Vector3::zero();
	}

	getCamera()->setCameraType(Camera::FIXED_CAMERA);
	getCamera()->setCameraFocus(G3D::CoordinateFrame(G3D::Matrix3::identity(), center));
	ServiceProvider::create<Soundscape::SoundService>(workspace)->playSound(STEP_SOUND);
	dataState->clearStack();
	dataState->setDirty(true);
	dataState->clearStack();
	dataState->setDirty(true);
}

void CanNotSelectCommand::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);
	FilteredSelection<PVInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;
	std::for_each(selection->begin(), selection->end(), &lockInstance);
	filteredSelection->clearSelection();
	dataState->clearStack();
	dataState->setDirty(true);
}

bool RunCommand::isEnabled() const
{
	return runService->getRunState() != RS_RUNNING;
}

void RunCommand::doIt(IDataState* dataState)
{
	playActionSound();
	RunService* service = runService;
	service->run();
}

bool StopCommand::isEnabled() const
{
	return runService->getRunState() == RS_RUNNING;
}

void StopCommand::doIt(IDataState* dataState)
{
	playActionSound();
	RunService* service = runService;
	service->pause();
}

bool ResetCommand::isEnabled() const
{
	RunState state = runService->getRunState();
	return (state == RS_RUNNING || state == RS_PAUSED) && !Network::Players::findLocalPlayer(dataModel);
}

void ResetCommand::doIt(IDataState* dataState)
{
	playActionSound();
	RunService* service = runService;
	service->reset();
}

bool EditSelectionVerb::isEnabled() const
{
	return selection->size() > 0;
}

bool GroupSelectionVerb::isEnabled() const
{
	if (selection->size() == 0) {
		return false;
	}

	ServiceClient<Selection> currentSelection(dataModel);
	return currentSelection->size() > 1;
}

void WeldSelectionVerb::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);
	Selection* currentSelection = dataModel ? dataModel->create<Selection>() : nullptr;
	std::for_each(selection->begin(), selection->end(), gluesToWelds);
	(dataModel ? dataModel->create<Soundscape::SoundService>() : nullptr)->playSound(PING_SOUND);
	dataState->clearStack();
	dataState->setDirty(true);
}

bool WeldSelectionVerb::isEnabled() const
{
	if (!EditSelectionVerb::isEnabled()) {
		return false;
	}

	return (dataModel ? dataModel->create<Selection>() : nullptr)->size() > 0;
}

bool SnapSelectionVerb::isEnabled() const
{
	if (!EditSelectionVerb::isEnabled()) {
		return false;
	}

	return (dataModel ? dataModel->create<Selection>() : nullptr)->size() > 0;
}

void SnapSelectionVerb::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);
	Selection* currentSelection = dataModel ? dataModel->create<Selection>() : nullptr;
	std::for_each(selection->begin(), selection->end(), weldsToStuds);
	(dataModel ? dataModel->create<Soundscape::SoundService>() : nullptr)->playSound(PING_SOUND);
	dataState->clearStack();
	dataState->setDirty(true);
}

bool UngroupSelectionVerb::isEnabled() const
{
	if (selection->size() == 0) {
		return false;
	}

	FilteredSelection<ModelInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<ModelInstance>>() : nullptr;
	return filteredSelection->findFirst(&hasChildren) != nullptr;
}

void DuplicateSelectionVerb::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	UndoState* undoState = new UndoState();
	RedoState* redoState = new RedoState();

	std::unique_ptr<XmlElement> root(SerializerV2::newRootElement());
	Selection* currentSelection = dataModel ? dataModel->create<Selection>() : nullptr;
	std::for_each(
		selection->begin(), selection->end(), [container = root.get()](const std::shared_ptr<Instance>& instance) {
			appendInstanceXml(container, instance);
		}
	);
	SerializerV2::isolateHandles(root.get());

	std::vector<std::shared_ptr<Instance>> instances;
	workspace->insertItems(root.get(), instances, INSERT_TO_3D_VIEW, ALLOW_PROMPTS);

	const std::vector<std::shared_ptr<Instance>>& items = instances;
	selection->setSelection(items.begin(), items.end());

	(dataModel ? dataModel->create<Soundscape::SoundService>() : nullptr)->playSound(PING_SOUND);

	for (std::vector<std::shared_ptr<Instance>>::const_iterator iter = items.begin(); iter != items.end(); ++iter) {
		if (Instance* instance = iter->get()) {
			undoState->addDelete(*instance);
			redoState->addParentProperty(*instance);
		}
	}

	dataState->pushState(getName(), undoState, redoState);
	dataState->setDirty(true);
}

void DeleteBase::doIt(IDataState* dataState)
{
	checkEditPermission(dataModel);

	if (selection->size() > 0) {
		Selection* currentSelection = dataModel ? dataModel->create<Selection>() : nullptr;

		UndoState* undoState = new UndoState();
		RedoState* redoState = new RedoState();

		std::for_each(
			selection->begin(), selection->end(), [undoState, redoState](const std::shared_ptr<Instance>& instance) {
				recordDelete(undoState, redoState, instance);
			}
		);

		while (std::shared_ptr<Instance> instance = selection->front()) {
			instance->setParent(nullptr);
		}

		(dataModel ? dataModel->create<Soundscape::SoundService>() : nullptr)->playSound(PAGE_SOUND);

		if (undoState) {
			dataState->pushState(getName(), undoState, redoState);
		}
		else {
			dataState->clearStack();
		}

		dataState->setDirty(true);
	}
}

bool AttachCameraCommand::isEnabled() const
{
	if (selection->size() == 0) {
		return false;
	}

	bool result;
	{
		ServiceClient<FilteredSelection<PVInstance>> filteredSelection(dataModel);
		result = filteredSelection->items().size() == 1;
	}

	return result ? true : false;
}

bool TrackCameraCommand::isEnabled() const
{
	if (!EditSelectionVerb::isEnabled()) {
		return false;
	}

	return (dataModel ? dataModel->create<Selection>() : nullptr)->size() == 1;
}

bool FollowCameraCommand::isEnabled() const
{
	if (selection->size() > 0) {
		FilteredSelection<PVInstance>* filteredSelection =
			dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;

		if (filteredSelection->items().size() == 1) {
			return true;
		}
	}

	return false;
}

bool WatchCameraCommand::isEnabled() const
{
	if (selection->size() > 0) {
		FilteredSelection<PVInstance>* filteredSelection =
			dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;

		if (filteredSelection->items().size() == 1) {
			return true;
		}
	}

	return false;
}

bool ControllerCommand::isEnabled() const
{
	if (selection->size() == 0) {
		return false;
	}

	FilteredSelection<PVInstance>* filteredSelection =
		dataModel ? dataModel->create<FilteredSelection<PVInstance>>() : nullptr;
	return filteredSelection->findFirst(&isControllable) != nullptr;
}

void CameraZoomExtentsCommand::doIt(IDataState* dataState)
{
	ServiceClient<FilteredSelection<PVInstance>> filteredSelection(workspace);
	Extents extents;

	if (filteredSelection->items().size() >= 1) {
		const std::vector<PVInstance*>& items = filteredSelection->items();
		extents = items[0]->getExtentsWorld();

		for (unsigned int i = 1; i < items.size(); i++) {
			extents.unionWith(items[i]->getExtentsWorld());
		}

		getCamera()->setCameraType(Camera::FIXED_CAMERA);
		getCamera()->zoomExtents(extents, workspace->getViewPort(), Camera::ZOOM_IN_OR_OUT);
		ServiceProvider::create<Soundscape::SoundService>(workspace)->playSound(STEP_SOUND);
		dataState->clearStack();
		dataState->setDirty(true);
		dataState->clearStack();
		dataState->setDirty(true);
	}
	else {
		G3D::Vector3 center = G3D::Vector3::zero();
		getCamera()->zoomExtents(workspace->getViewPort());
		ServiceProvider::create<Soundscape::SoundService>(workspace)->playSound(STEP_SOUND);
		dataState->clearStack();
		dataState->setDirty(true);
	}
}

bool CameraPanLeftCommand::isEnabled() const
{
	return true;
}

bool CameraPanRightCommand::isEnabled() const
{
	return true;
}

bool FixedCameraCommand::isEnabled() const
{
	return true;
}

void FirstPersonCommand::doIt(IDataState* dataState)
{
}

void ForceAssertionCommand::doIt(IDataState* dataState)
{
}

bool EngineStatsCommand::isEnabled() const
{
	return true;
}

} // namespace RBX
