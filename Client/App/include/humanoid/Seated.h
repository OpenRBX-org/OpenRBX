#ifndef HUMANOID_SEATED_H
#define HUMANOID_SEATED_H

#include "humanoid/Humanoid.h"
#include "util/Name.h"

namespace RBX {

extern const char sSeated[];

class Seated : public Named<Humanoid::State, sSeated>
{
private:
	State* onStep(const float stepDt, Controller& controller) override;

	void onComputeForce(const float dt) override {}

public:
	Seated(Humanoid* humanoid);

	~Seated() override;
};

} // namespace RBX

#endif // HUMANOID_SEATED_H
