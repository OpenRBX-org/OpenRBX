#ifndef V8DATAMODEL_ICHARACTERSUBJECT_H
#define V8DATAMODEL_ICHARACTERSUBJECT_H

#include "util/ExponentialRunningAverage.h"
#include "util/ICameraSubject.h"

#include <G3D/Vector3.h>

namespace RBX {

class ContactManager;

class ICharacterSubject : public ICameraSubject
{
private:
	Vector3ERA focusERA;
	Vector3ERA focusOffsetERA;
	G3D::Vector3 lastOffset;
	floatERA rotationERA;
	bool cursorLocked;
	bool wasOccluded;
	float unoccludedDistance;
	bool hasGoalInFocus;
	G3D::Vector3 goalInFocus;

	CoordinateFrame getFocusLookingAtGoal(const CoordinateFrame& cameraFocus, const CoordinateFrame& cameraGoal);
	bool testOcclusion(const CoordinateFrame& cameraFocus, const CoordinateFrame& cameraGoal, float& distance);
	CoordinateFrame goalFromDistance(
		const CoordinateFrame& cameraFocus, const CoordinateFrame& cameraGoal, const float distance
	);

	bool zoom(const float in, CoordinateFrame& cameraGoal, CoordinateFrame& cameraFocus) override;
	void stepGoalAndFocus(
		CoordinateFrame& cameraGoal, CoordinateFrame& cameraFocus, bool cameraExternallyAdjusted
	) override;

public:
	ICharacterSubject();

	virtual ContactManager* getContactManager() = 0;
	virtual void getIgnorePrims(std::vector<const Primitive*>& prims) = 0;
	virtual void tellCameraNear(float distance) = 0;
	virtual G3D::Vector3 getIntendedMovementVector() = 0;
	virtual float getIntendedRotationAboutYAxis() = 0;
	virtual void cameraSetWalkOrientation(float angle, bool value) = 0;

	void onHeartBeat(CoordinateFrame& cameraGoal, CoordinateFrame& cameraFocus);
	float panAdjust();
	void setCursorCenterLocked(bool value);
	bool getCursorCenterLocked() const { return cursorLocked; }
};

} // namespace RBX

#endif // V8DATAMODEL_ICHARACTERSUBJECT_H
