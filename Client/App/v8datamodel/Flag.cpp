#include "v8datamodel/Flag.h"

#include "humanoid/Humanoid.h"
#include "network/Player.h"
#include "network/Players.h"
#include "reflection/property.h"
#include "util/RunStateOwner.h"
#include "v8datamodel/FlagStand.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/TimerService.h"

namespace RBX {

const char sFlag[] = "Flag";

static Reflection::PropDescriptor<Flag, BrickColor> prop_Color(
	"TeamColor", "Data", &Flag::getTeamColor, &Flag::setTeamColor
);

BrickColor Flag::getTeamColor() const
{
	return teamColor;
}

bool Flag::canBePickedUpByPlayer(Network::Player* p)
{
	if (p->getNeutral()) {
		return false;
	}

	return p->getTeamColor() == getTeamColor() ? false : true;
}

Flag::~Flag() = default;

void Flag::doUglyPeriodicCloneHack()
{
	evilClone = clone();

	if (timer && numChildren() == 0) {
		timer->delay([self = shared_from(this)] { self->doUglyPeriodicCloneHack(); }, 2.0);
	}
}

void Flag::onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider)
{
	Instance::onServiceProvider(oldProvider, newProvider);

	if (oldProvider == nullptr) {
		timer = newProvider ? newProvider->create<TimerService>() : nullptr;
		timer->delay([self = shared_from(this)] { self->doUglyPeriodicCloneHack(); }, 2.0);
	}

	if (newProvider == nullptr) {
		flagTouched.disconnect();

		RunService* runService = oldProvider ? oldProvider->create<RunService>() : nullptr;
		if (runService->getRunState() == RS_RUNNING && evilClone) {
			Flag* flag = dynamic_cast<Flag*>(evilClone.get());
			flag->getHandle();

			FlagStandService* flagStandService = ServiceProvider::create<FlagStandService>(this);
			if (!flagStandService->FindStandWithFlag(flag)) {
				if (FlagStand* stand = flagStandService->FindClosestEmptyStandForFlag(flag)) {
					stand->AffixFlag(flag);
				}
			}
		}
	}
}

void Flag::onEvent_flagTouched(std::shared_ptr<Instance> other)
{
	Instance* character = other->getParent();
	if (!Humanoid::modelIsCharacter(character)) {
		return;
	}

	Network::Player* player = Network::Players::getPlayerFromCharacter(character);
	if (!player) {
		return;
	}
	if (player->getNeutral()) {
		return;
	}
	if (player->getTeamColor() != getTeamColor()) {
		return;
	}

	FlagStandService* flagStandService = ServiceProvider::create<FlagStandService>(this);
	if (flagStandService->FindStandWithFlag(this)) {
		return;
	}

	FlagStand* stand = flagStandService->FindClosestEmptyStandForFlag(this);
	if (stand) {
		stand->AffixFlag(this);
	}
}

void Flag::onChildAdded(Instance* instance)
{
	if (dynamic_cast<PartInstance*>(instance)) {
		if (PartInstance* handle = getHandle()) {
			flagTouched = PartInstance::event_Touched.connect(
				handle, [this](const std::shared_ptr<Instance>& other) { onEvent_flagTouched(other); }, 10
			);
		}
	}
}

Flag::Flag() : timer(nullptr)
{
	setName(sFlag);
}

void Flag::setTeamColor(BrickColor color)
{
	teamColor = color;
	raisePropertyChanged(prop_Color);
}

} // namespace RBX

template class RBX::FactoryProduct<RBX::Flag, RBX::Tool, RBX::sFlag>;
template class RBX::Reflection::Described<RBX::Flag, RBX::sFlag, RBX::FactoryProduct<RBX::Flag, RBX::Tool, RBX::sFlag>>;
template class RBX::DescribedCreatable<RBX::Flag, RBX::Tool, RBX::sFlag>;
