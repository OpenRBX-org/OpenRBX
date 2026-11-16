#ifndef V8WORLD_JOINTSTAGE_H
#define V8WORLD_JOINTSTAGE_H

#include "v8world/IWorldStage.h"

#include <map>
#include <set>

namespace RBX {

class ClumpStage;
class Joint;
class Primitive;

class JointStage : public IWorldStage
{
public:
	using JointMap = std::multimap<Primitive*, Joint*>;

	JointStage(IStage* upstream, World* world);

	~JointStage() override = default;

	StageType getStageType() override { return JOINT_STAGE; }

	void onEdgeAdded(Edge* edge) override;
	void onEdgeRemoving(Edge* edge) override;

	void onPrimitiveAdded(Primitive* p);
	void onPrimitiveRemoving(Primitive* p);

	void onJointPrimitiveNulling(Joint* j, Primitive* nulling);
	void onJointPrimitiveSet(Joint* j, Primitive* p);

private:
	ClumpStage* getClumpStage();

	void moveEdgeToDownstream(Edge* e);
	void removeEdgeFromDownstream(Edge* e);

	void moveJointToDownstream(Joint* j);
	void removeJointFromDownstream(Joint* j);

	bool jointInList(Joint* j);
	void removeFromList(Joint* j);
	void insertToList(Joint* j);

	bool pairInMap(Joint* j, Primitive* p);
	void insertToMap(Joint* j, Primitive* p);
	void removeFromMap(Joint* j, Primitive* p);

	bool edgeHasPrimitivesDownstream(Edge* e);

	std::set<Joint*> incompleteJoints;
	JointMap jointMap;
};

} // namespace RBX

#endif // V8WORLD_JOINTSTAGE_H
