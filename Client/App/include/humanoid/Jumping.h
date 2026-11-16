#ifndef HUMANOID_JUMPING_H
#define HUMANOID_JUMPING_H

#include "humanoid/Flying.h"
#include "util/Name.h"

namespace RBX {

extern const char sJumping[];

class Jumping : public Named<Flying, sJumping>
{
private:
	bool done;

	static const float kJumpP;
	static const float jumpVelocityGrid;

	float timeout;

	State* onStep(const float stepDt, Controller& controller) override;
	void onComputeForce(const float dt) override;

public:
	Jumping(Humanoid* humanoid);

	~Jumping() override;
};

} // namespace RBX

#endif // HUMANOID_JUMPING_H
