#ifndef V8DATAMODEL_SPAWNLOCATION_H
#define V8DATAMODEL_SPAWNLOCATION_H

#include "reflection/property.h"
#include "util/Handle.h"
#include "util/Signals.h"
#include "v8datamodel/BrickColor.h"
#include "v8datamodel/PartInstance.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"

#include <list>
#include <memory>

namespace RBX {

class ModelInstance;

namespace Network {
class Player;
}

extern const char sSpawnerService[];

extern char sSpawnLocation[];

class SpawnLocation : public DescribedCreatable<SpawnLocation, PartInstance, sSpawnLocation>
{
private:
	BrickColor teamColor;
	Signals::ScopedConnection spawnerTouched;

	void onEvent_spawnerTouched(std::shared_ptr<Instance> other);

public:
	SpawnLocation();

	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;

	bool neutral;
	bool allowTeamChangeOnTouch;
	int forcefieldDuration;

	static Reflection::BoundProp<bool, 1> prop_Neutral;
	static Reflection::BoundProp<bool, 1> prop_AllowTeamChangeOnTouch;
	static Reflection::BoundProp<int, 1> prop_SpawnForcefieldDuration;

	BrickColor getTeamColor() const;
	void setTeamColor(BrickColor value);
};

class SpawnerService : public DescribedCreatable<SpawnerService, Instance, sSpawnerService>, public Service
{
public:
	SpawnerService();
	~SpawnerService() override;

	void RegisterSpawner(SpawnLocation* spawner) { spawners.push_back(spawner); }
	void UnregisterSpawner(SpawnLocation* spawner) { spawners.remove(spawner); }
	void SpawnPlayer(std::shared_ptr<ModelInstance> model, Network::Player* p);

private:
	std::list<SpawnLocation*> spawners;
};

} // namespace RBX

#endif // V8DATAMODEL_SPAWNLOCATION_H
