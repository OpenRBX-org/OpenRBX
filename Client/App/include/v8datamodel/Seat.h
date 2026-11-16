#ifndef V8DATAMODEL_SEAT_H
#define V8DATAMODEL_SEAT_H

#include "util/Signals.h"
#include "v8datamodel/PartInstance.h"
#include "v8tree/Instance.h"

#include <memory>

namespace RBX {

class Humanoid;
class Weld;

extern char sSeat[];
class Seat : public DescribedCreatable<Seat, PartInstance, sSeat>
{
private:
	Signals::ScopedConnection seatTouched;
	Signals::ScopedConnection humanoidJumped;

	void onEvent_seatTouched(std::shared_ptr<Instance> other);
	void onEvent_humanoidJumped(bool active);
	void seatCharacter(Humanoid* h);
	void unseatCharacter();

	std::shared_ptr<Weld> weld;

	bool charSeated();

	double sleepTime;

public:
	Seat();

	virtual void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider);
};

} // namespace RBX

#endif // V8DATAMODEL_SEAT_H
