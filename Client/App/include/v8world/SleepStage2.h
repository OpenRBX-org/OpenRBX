#ifndef V8WORLD_SLEEPSTAGE2_H
#define V8WORLD_SLEEPSTAGE2_H

#include "util/IndexArray.h"
#include "util/Profiling.h"
#include "v8world/Assembly2.h"
#include "v8world/Contact.h"
#include "v8world/IWorldStage.h"

#include <G3D/Array.h>
#include <memory>
#include <set>
#include <vector>

namespace RBX {

class Joint;
class SimJobStage;

class SleepStage : public IWorldStage
{
public:
	using AssemblySet = std::set<Assembly*>;
	using JointSet = std::set<Joint*>;
	using AssemblySetIt = AssemblySet::iterator;
	using CAssemblySetIt = AssemblySet::const_iterator;
	using ContactList = IndexArray<Contact, &Contact::steppingIndexFunc>;

	SleepStage(IStage* upstream, World* world);

	~SleepStage() override;

	StageType getStageType() override { return SLEEP_STAGE; }

	void stepWorld(int worldStepId, int _uiStepId, bool _throttling) override;

	void onEdgeAdded(Edge* edge) override;
	void onEdgeRemoving(Edge* edge) override;
	int getMetric(MetricType metricType) override;

	void onAssemblyAdded(Assembly* assembly);
	void onAssemblyRemoving(Assembly* assembly);

	void onWakeUpRequest(Assembly* assembly, bool recursive);

	void onLosingContact(const G3D::Array<Contact*>& contacts);

	void addedBodyForce() { externalBodyForceAdded = true; }

	int numTouchingContacts() { return touchingContacts.size(); }

	const AssemblySet& getAwakeAssemblies() const { return awake; }

private:
	static float sleepTolerance() { return 0.02f; }
	static int stepsToSleep() { return 6; }

	bool isMoving(Assembly* assembly);
	bool canThrottle(Assembly* assembly);
	bool atLeastOneAssemblyMoving(Assembly* a0, Assembly* a1);

	bool edgeIsAwake(Edge* edge);
	bool isAffecting(Edge* edge);

	bool computeShouldSleep(Assembly* a);
	bool computeCanSleep(Assembly* assembly);
	bool shouldSleep(Assembly* assembly);
	bool okNeighborSleep(Assembly* assembly);
	bool preventNeighborSleep(Assembly* assembly);
	bool forceNeighborAwake(Assembly* assembly);

	Sim::AssemblyState computeAssemblyState(Assembly* assembly);

	Sim::AssemblyState getState(Assembly* assembly);
	AssemblySet* stateToSet(Sim::AssemblyState state);

	int getSleepCount(Assembly* assembly);
	void incrementSleepCount(Assembly* assembly);
	void resetSleepCount(Assembly* assembly);

	void changeAssemblyState(Assembly* assembly, Sim::AssemblyState state);
	void changeAssemblyState(const std::vector<Assembly*>& assemblies, Sim::AssemblyState state);
	void changeContactState(Contact* contact, Sim::EdgeState state);
	void changeContactState(const std::vector<Contact*>& contacts, Sim::EdgeState state);
	void changeJointState(Joint* joint, Sim::EdgeState state);
	void changeJointState(const std::vector<Joint*>& joints, Sim::EdgeState state);

	void wakeAssembly(Assembly* assembly);
	void wakeAssemblyAndNeighbors(Assembly* assembly);
	void wakeAssemblyAndNeighbors(Assembly* assembly, int depth);
	void wakeEdge(Edge* edge);

	Sim::EdgeState computeContactState(bool moving, bool touching, bool canCollide, bool wasTouching);

	void wakeEvent(Assembly* assembly);
	void wakeEvent(Edge* edge);
	void recursiveWakeEvent(Assembly* assembly);
	void touchEvent(Contact* contact);

	void stepContacts(ContactList& contacts);
	void stepJoints();
	void stepAssembliesAwake();
	void stepAssembliesSleepingChecking();
	void stepAssembliesWakePending();
	void stepAssembliesRecursiveWakePending();

	bool validate();

	bool externalBodyForceAdded;
	int numContactsInStage;
	int numContactsInKernel;
	bool throttling;
	int uiStepId;
	AssemblySet recursiveWakePending;
	AssemblySet wakePending;
	ContactList steppingContacts;
	ContactList touchingContacts;
	JointSet steppingJoints;
	AssemblySet awake;
	AssemblySet sleepingChecking;
	AssemblySet sleepingDeeply;

public:
	std::unique_ptr<Profiling::CodeProfiler> profilingCollision;
	std::unique_ptr<Profiling::CodeProfiler> profilingWake;
	std::unique_ptr<Profiling::CodeProfiler> profilingSleep;
};

} // namespace RBX

#endif // V8WORLD_SLEEPSTAGE2_H
