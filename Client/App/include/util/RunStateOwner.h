#ifndef UTIL_RUNSTATEOWNER_H
#define UTIL_RUNSTATEOWNER_H

#include "util/Events.h"
#include "util/Handle.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"

#include <atomic>
#include <condition_variable>
#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace RBX {

class DataModel;

enum RunState
{
	RS_NORMAL = 0,
	RS_RUNNING = 1,
	RS_PAUSED = 2
};

class Stepped
{
public:
	const float time;
	const float step;

	Stepped(float time, float step) : time(time), step(step) {}
};

class Heartbeat
{
public:
	const float time;
	const float step;

	Heartbeat(float time, float step) : time(time), step(step) {}
};

class RunTransition
{
public:
	RunState oldState;
	RunState newState;

	RunTransition(RunState oldState, RunState newState) : oldState(oldState), newState(newState) {}

	bool startEvent();
	bool resetEvent();
};

class IRunView
{
public:
	virtual void InvalidateRunView() = 0;
	virtual void AdjustThrottle(double frameRate) = 0;
};

extern const char sRunService[];

class RunService : public DescribedCreatable<RunService, Instance, sRunService>,
				   public Notifier<RunService, Heartbeat>,
				   public Notifier<RunService, Stepped>,
				   public Notifier<RunService, RunTransition>,
				   public Notifier<RunService, RunState>,
				   public Service
{
private:
	using Views = std::map<IRunView*, bool>;

	RunState runState;
	std::mutex runMutex;
	std::atomic<bool> stopRequested;
	std::condition_variable runViewsDoneCondition;
	std::condition_variable stateChangedCondition;
	float framePeriod;
	std::thread runThread;
	std::recursive_mutex viewMutex;
	Views views;
	std::atomic<int> invalidRunViewCount;
	int stepCount;
	double stepTime;

	void start();

	friend class DataModel;

public:
	bool runDisabled;

	RunService();

	~RunService() override;

	void setRunState(RunState state);

	void run()
	{
		if (!runDisabled) {
			setRunState(RS_RUNNING);
		}
	}

	void pause() { setRunState(RS_PAUSED); }

	void reset() { setRunState(RS_NORMAL); }

	void endRunThread(bool join);

	void raiseHeartbeat(float time, float step);
	void raiseStepped(float time, float step);

	RunState getRunState() const { return runState; }

	bool isEditState() const { return runState == RS_NORMAL; }
	bool isRunState() const { return runState == RS_RUNNING; }
	bool isPauseState() const { return runState == RS_PAUSED; }

	void setPeriod(float period) { framePeriod = period; }
	float getPeriod() { return framePeriod; }

	int getStepCount() const { return stepCount; }
	double getStepTime() const { return stepTime; }

	void addRunView(IRunView* runView);
	void removeRunView(IRunView* runView);
	void invalidateRunViews();
	void runViewValid(IRunView* runView);

	static Reflection::SignalDesc<RunService, void(float, float)> event_Stepped;

	void onAncestorChanged(const AncestorChanged& event) override;

private:
	void runProc(std::shared_ptr<DataModel> dataModel);
};

} // namespace RBX

#endif // UTIL_RUNSTATEOWNER_H
