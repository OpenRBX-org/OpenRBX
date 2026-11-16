#ifndef V8DATAMODEL_FLAG_H
#define V8DATAMODEL_FLAG_H

#include "util/Signals.h"
#include "v8datamodel/BrickColor.h"
#include "v8datamodel/Tool.h"
#include "v8tree/Instance.h"

#include <memory>

namespace RBX {

class TimerService;

extern const char sFlag[];

class Flag : public DescribedCreatable<Flag, Tool, sFlag>
{
public:
	enum FlagState
	{
		NOTHING = 0,
		LOOSE = 1,
		IN_STAND = 2,
		EQUIPPED = 3
	};

private:
	std::shared_ptr<Instance> evilClone;
	FlagState state;
	Signals::ScopedConnection flagTouched;

	void onEvent_flagTouched(std::shared_ptr<Instance> other);

	TimerService* timer;

	void doUglyPeriodicCloneHack();

protected:
	bool canUnequip() override { return false; }

	bool canBePickedUpByPlayer(Network::Player* p) override;

public:
	BrickColor teamColor;

	BrickColor getTeamColor() const;

	void setTeamColor(BrickColor value);

	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;

	Flag();
	~Flag() override;

	void onChildAdded(Instance* child) override;
};

} // namespace RBX

#endif // V8DATAMODEL_FLAG_H
