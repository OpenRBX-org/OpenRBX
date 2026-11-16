#ifndef HUMANOID_RUNNING_H
#define HUMANOID_RUNNING_H

#include "humanoid/Balancing.h"
#include "util/IRenderable.h"
#include "util/Name.h"

#include <G3D/Vector3.h>

namespace RBX {

class Body;

extern const char sRunning[];

class Running : public Named<Balancing, sRunning>, public IRenderable
{
private:
	G3D::Vector3 floorVelocity;
	float floorRotationalVelocity;
	float jumpWarmup;
	bool lastLadderResult;
	double lastLadderTime;
	G3D::Vector3 desiredVelocity;
	float desiredYRotationalVelocity;
	float desiredAltitude;
	float intendedYRotation;

	void rotateWithGround(Body* body);
	void hoverOnFloor(Body* body);
	void move(Body* body);

	static const float manualTurnSpeed;
	static const float autoTurnSpeed;
	static const float kTurnP;
	static const float kTurnAccelMax;
	static const float kMoveP;
	static const float kAltitudeP;
	static const float kAltitudeD;
	static const G3D::Vector3 maxMoveAccelerationGrid;

	bool isFacingStairs();
	bool isFacingLadder(bool buffered);
	void updateGoals(Controller& controller);

	void onComputeForce(const float dt) override;
	State* onStep(const float stepDt, Controller& controller) override;

	float getIntendedRotationAboutYAxis() const override { return intendedYRotation; }

public:
	Running(Humanoid* humanoid);

	~Running() override;

	bool shouldRender2d() const override // IRenderable vtable+0x00
	{
		return false;
	}

	bool shouldRender3dAdorn() const override // IRenderable vtable+0x04
	{
		return true;
	}

	void render3dAdorn(Adorn* adorn) override; // IRenderable vtable+0x0c
};

} // namespace RBX

#endif // HUMANOID_RUNNING_H
