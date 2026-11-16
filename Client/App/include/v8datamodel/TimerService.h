#ifndef V8DATAMODEL_TIMERSERVICE_H
#define V8DATAMODEL_TIMERSERVICE_H

#include "util/Handle.h"
#include "util/RunStateOwner.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"

#include <functional>
#include <list>
#include <memory>

namespace RBX {

extern char sTimerService[];
class TimerService : public DescribedCreatable<TimerService, Instance, sTimerService>,
					 public Service,
					 public Listener<RunService, Heartbeat>
{
private:
	std::shared_ptr<RunService> runService;

	class Item
	{
	public:
		double time;
		std::function<void()> func;
	};

	std::list<Item> items;

public:
	TimerService();

	void delay(std::function<void()> func, double seconds);

protected:
	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;
	void onEvent(const RunService* source, Heartbeat event) override;

public:
};

} // namespace RBX

#endif // V8DATAMODEL_TIMERSERVICE_H
