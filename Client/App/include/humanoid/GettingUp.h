#ifndef HUMANOID_GETTINGUP_H
#define HUMANOID_GETTINGUP_H

#include "humanoid/Balancing.h"
#include "util/Name.h"

namespace RBX {

extern const char sGettingUp[];

class GettingUp : public Named<Balancing, sGettingUp>
{
protected:
	State* onStep(const float stepDt, Controller& controller) override;

public:
	GettingUp(Humanoid* humanoid);

	~GettingUp() override;
};

} // namespace RBX

#endif // HUMANOID_GETTINGUP_H
