#ifndef V8WORLD_ASSEMBLYSTAGE2_H
#define V8WORLD_ASSEMBLYSTAGE2_H

#include "v8world/IWorldStage.h"

#include <set>

namespace RBX {

class Assembly;
class Joint;
class SleepStage;

class AssemblyStage : public IWorldStage
{
public:
	AssemblyStage(IStage* upstream, World* world);

	~AssemblyStage() override;

	StageType getStageType() override;

	void onEdgeAdded(Edge* edge) override;
	void onEdgeRemoving(Edge* edge) override;

	void onAssemblyAdded(Assembly* assembly);
	void onAssemblyRemoving(Assembly* assembly);

	void wakeAssembly(Assembly* assembly);

	void stepUi(int uiStepId);

private:
	void onJointAdded(Joint* joint);
	void onJointRemoving(Joint* joint);

	std::set<Assembly*> assemblies;
	std::set<Joint*> joints;
};

} // namespace RBX

#endif // V8WORLD_ASSEMBLYSTAGE2_H
