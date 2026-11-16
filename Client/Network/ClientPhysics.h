#ifndef NETWORK_CLIENTPHYSICS_H
#define NETWORK_CLIENTPHYSICS_H

#include "util/RunStateOwner.h"

#include <G3D/CoordinateFrame.h>
#include <memory>

namespace RBX {

class PartInstance;

namespace Network {

class ClientPhysics : public Listener<RunService, Heartbeat>
{
private:
	double lastUpdateTime;
	std::weak_ptr<RunService> runService;
	PartInstance* part;
	G3D::CoordinateFrame lastPos;

public:
	ClientPhysics(PartInstance* part);

	void update(const std::shared_ptr<RunService>& runService);
	void sleep();

	virtual ~ClientPhysics();

protected:
	virtual void onEvent(const RunService* source, Heartbeat event);
};

} // namespace Network
} // namespace RBX

#endif // NETWORK_CLIENTPHYSICS_H
