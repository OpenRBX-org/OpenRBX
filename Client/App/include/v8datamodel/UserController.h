#ifndef V8DATAMODEL_USERCONTROLLER_H
#define V8DATAMODEL_USERCONTROLLER_H

#include "util/Events.h"
#include "util/Handle.h"
#include "util/RunStateOwner.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"
#include "v8world/Controller.h"

#include <G3D/ReferenceCount.h>
#include <G3D/Vector3.h>
#include <memory>

namespace RBX {

class PVInstance;
class UserInputBase;

extern char sControllerService[];
class ControllerService : public DescribedCreatable<ControllerService, Instance, sControllerService>, public Service
{
private:
	UserInputBase* hardwareDevice;
	G3D::ReferenceCountedPointer<Controller> playerController;
	G3D::ReferenceCountedPointer<Controller> keyboardPrimaryController;
	G3D::ReferenceCountedPointer<Controller> keyboardSecondaryController;
	G3D::ReferenceCountedPointer<Controller> nullController;

public:
	ControllerService();

	G3D::ReferenceCountedPointer<Controller> getController(
		Controller::ControllerType type, const PVInstance* controlledInstance
	);

	const UserInputBase* getHardwareDevice() const { return hardwareDevice; }
	void setHardwareDevice(UserInputBase* device) { hardwareDevice = device; }
	UserInputBase* getHardwareDevice() { return hardwareDevice; }
};

class SteppingController : public Controller, public Listener<RunService, Heartbeat>
{
private:
	std::shared_ptr<RunService> runService;

protected:
	std::weak_ptr<const PVInstance> controlledInstance;

	SteppingController(const PVInstance* _controlledInstance);
	~SteppingController() override;
};

class AIController : public SteppingController
{
private:
	float retargetTime;

protected:
	float buffer[NUM_INPUT_TYPES];
	std::weak_ptr<const PVInstance> target;

	AIController(const PVInstance* controlledInstance);
	~AIController() override;

	void updateTarget(ControllerType excludeType, float time);
	G3D::Vector3 getLocalTargetPosition() const;
	void chaseTarget(const G3D::Vector3& localChasePoint);

	bool hasIntelligence() const override { return true; }
	bool isUserController() const override { return false; }
	float getValue(InputType inputType) const override;
	virtual void updateBuffer(float time) = 0;

	void onEvent(const RunService* source, Heartbeat event) override { updateBuffer(event.time); }
};

class AIChaseController : public AIController
{
protected:
	void updateBuffer(float time) override;

public:
	AIChaseController(const PVInstance* controlledInstance) : AIController(controlledInstance) {}

	ControllerType getControllerType() const override { return AI_CHASE_CONTROLLER; }
};

class AIFleeController : public AIController
{
protected:
	void updateBuffer(float time) override;

public:
	AIFleeController(const PVInstance* controlledInstance) : AIController(controlledInstance) {}

	ControllerType getControllerType() const override { return AI_FLEE_CONTROLLER; }
};

class HardwareController : public Controller
{
private:
	ControllerService* controllerService;

protected:
	HardwareController(ControllerService* _controllerService) : controllerService(_controllerService) {}

	const UserInputBase* getHardwareDevice() const
	{
		return controllerService ? controllerService->getHardwareDevice() : nullptr;
	}

	bool hasIntelligence() const override { return true; }
	bool isUserController() const override { return true; }
};

class KeyboardPrimaryController : public HardwareController
{
public:
	KeyboardPrimaryController(ControllerService* _controllerService) : HardwareController(_controllerService) {}

	float getValue(InputType inputType) const override;
	ControllerType getControllerType() const override { return PRIMARY_CONTROLLER; }
};

class KeyboardSecondaryController : public HardwareController
{
public:
	KeyboardSecondaryController(ControllerService* _controllerService) : HardwareController(_controllerService) {}

	float getValue(InputType inputType) const override;
	ControllerType getControllerType() const override { return SECONDARY_CONTROLLER; }
};

class PlayerController : public SteppingController
{
private:
	std::weak_ptr<ControllerService> controllerService;
	int stepsRotating;

	const UserInputBase* getHardwareDevice() const;

	bool hasIntelligence() const override { return true; }
	bool isUserController() const override { return true; }
	ControllerType getControllerType() const override { return PLAYER_CONTROLLER; }
	void onEvent(const RunService* source, Heartbeat event) override;

public:
	PlayerController(ControllerService* _controllerService, const PVInstance* _controlledInstance);
};

} // namespace RBX

#endif // V8DATAMODEL_USERCONTROLLER_H
