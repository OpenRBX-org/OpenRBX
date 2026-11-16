#include "util/RunStateOwner.h"

#include "util/Thread.h"
#include "util/Timer.h"
#include "v8datamodel/DataModel.h"

#include <algorithm>
#include <chrono>

namespace RBX {

const char sRunService[] = "RunService";

Reflection::SignalDesc<RunService, void(float, float)> RunService::event_Stepped("Stepped", "time", "step");

static Reflection::SignalDesc<RunService, void(float)> event_Heartbeat("Heartbeat", "interval");

static Reflection::BoundFuncDesc<RunService, void(), 0> runFunction(
	&RunService::run, "Run", Reflection::FunctionDescriptor::NeedTrustedCaller
);

static Reflection::BoundFuncDesc<RunService, void(), 0> pauseFunction(
	&RunService::pause, "Pause", Reflection::FunctionDescriptor::NeedTrustedCaller
);

static Reflection::BoundFuncDesc<RunService, void(), 0> resetFunction(
	&RunService::reset, "Reset", Reflection::FunctionDescriptor::NeedTrustedCaller
);

void RunService::endRunThread(bool join)
{
	{
		std::scoped_lock lock(runMutex);
		stopRequested = true;
	}

	stateChangedCondition.notify_all();

	if (join && runThread.joinable()) {
		runThread.join();
	}
}

void RunService::onAncestorChanged(const AncestorChanged& event)
{
	if (getParent() == nullptr) {
		endRunThread(false);
	}
}

void RunService::setRunState(RunState state)
{
	if (state == RS_RUNNING) {
		if (!isEditState() && !isPauseState()) {
			return;
		}
	}
	else if (state == RS_PAUSED) {
		if (!isRunState()) {
			return;
		}
	}

	if (runState == state) {
		return;
	}

	RunTransition transition(runState, state);

	runState = state;
	Notifier<RunService, RunState>::raise(state);
	Notifier<RunService, RunTransition>::raise(transition);
}

void RunService::runProc(std::shared_ptr<DataModel> dataModel)
{
	double desiredTime = getRealTime();

	while (true) {
		const double timeLeft = desiredTime - getRealTime();

		{
			std::scoped_lock lock(viewMutex);

			for (Views::iterator iter = views.begin(); iter != views.end(); ++iter) {
				iter->first->AdjustThrottle(timeLeft);
			}
		}

		const double waitTime = std::max(0.0, timeLeft);
		const std::chrono::steady_clock::time_point wakeTime =
			std::chrono::steady_clock::now() +
			std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(waitTime));

		bool signalled;

		{
			std::unique_lock lock(runMutex);

			signalled = stateChangedCondition.wait_until(lock, wakeTime, [this] { return stopRequested.load(); });
		}

		if (!signalled) {
			const std::chrono::steady_clock::time_point timeout =
				std::chrono::steady_clock::now() + std::chrono::seconds(1);

			std::unique_lock lock(runMutex);

			if (invalidRunViewCount != 0) {
				runViewsDoneCondition.wait_until(lock, timeout);
			}
		}

		if (timeLeft <= 0.0) {
			desiredTime = getRealTime();
		}

		{
			DataModel::Lock lock(dataModel);

			if (stopRequested) {
				break;
			}

			dataModel->step(framePeriod);

			stepCount++;
			stepTime = desiredTime;
		}

		desiredTime += framePeriod;
	}
}

void RunService::invalidateRunViews()
{
	std::scoped_lock lock(viewMutex);

	if (invalidRunViewCount != 0) {
		return;
	}

	invalidRunViewCount = static_cast<int>(views.size());

	if (invalidRunViewCount > 0) {
		for (Views::iterator iter = views.begin(); iter != views.end(); ++iter) {
			iter->first->InvalidateRunView();
			iter->second = false;
		}

		runViewsDoneCondition.notify_all();
	}
}

void RunService::runViewValid(IRunView* runView)
{
	std::scoped_lock lock(viewMutex);

	if (invalidRunViewCount == 0) {
		return;
	}

	Views::iterator iter = views.find(runView);

	if (iter != views.end() && !iter->second) {
		invalidRunViewCount--;
		iter->second = true;
	}

	if (invalidRunViewCount == 0) {
		runViewsDoneCondition.notify_all();
	}
}

RunService::~RunService()
{
	if (runThread.joinable()) {
		runThread.detach();
	}
}

void RunService::removeRunView(IRunView* runView)
{
	std::scoped_lock lock(viewMutex);

	runViewValid(runView);
	views.erase(runView);
}

void RunService::addRunView(IRunView* runView)
{
	std::scoped_lock lock(viewMutex);

	views[runView] = true;
}

RunService::RunService()
	: runState(RS_NORMAL), stopRequested(false), framePeriod(1.0f / 30.0f), invalidRunViewCount(0), stepCount(0),
	  stepTime(0.0), runDisabled(false)
{
	setName("Run Service");
}

void RunService::start()
{
	std::shared_ptr<DataModel> dataModel = shared_from(dynamic_cast<DataModel*>(getParent()));

	runThread = std::thread(
		background_function([self = shared_from(this), dataModel] { self->runProc(dataModel); }, "rbx_runProc")
	);
}

void RunService::raiseHeartbeat(float time, float step)
{
	Notifier<RunService, Heartbeat>::raise(Heartbeat(time, step));
	event_Heartbeat.fire(this, step);
}

void RunService::raiseStepped(float time, float step)
{
	Notifier<RunService, Stepped>::raise(Stepped(time, step));
	event_Stepped.fire(this, time, step);
}

} // namespace RBX

template class RBX::FactoryProduct<RBX::RunService, RBX::Instance, RBX::sRunService>;
template class RBX::Reflection::
	Described<RBX::RunService, RBX::sRunService, RBX::FactoryProduct<RBX::RunService, RBX::Instance, RBX::sRunService>>;
template class RBX::DescribedCreatable<RBX::RunService, RBX::Instance, RBX::sRunService>;
