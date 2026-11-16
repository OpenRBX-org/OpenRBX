#ifndef V8DATAMODEL_DEBRISSERVICE_H
#define V8DATAMODEL_DEBRISSERVICE_H

#include "util/Handle.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"

#include <memory>
#include <queue>

namespace RBX {

extern char sDebrisService[];

class TimerService;

class DebrisService : public DescribedCreatable<DebrisService, Instance, sDebrisService>, public Service
{
private:
	std::queue<std::weak_ptr<Instance> > queue;
	int maxItems;
	TimerService* timer;

public:
	DebrisService();

	void addItem(std::shared_ptr<Instance> item, double lifetime);
	void setMaxItems(int value);

	int getMaxItems() const { return maxItems; }

protected:
	virtual void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider);

private:
	void cleanup();
};

} // namespace RBX

#endif // V8DATAMODEL_DEBRISSERVICE_H
