#ifndef V8WORLD_MECHANISM_H
#define V8WORLD_MECHANISM_H

#include <cstddef>
#include <list>
#include <set>
#include <vector>

namespace RBX {

class Assembly;
class Mechanism;
class Primitive;

class MechanismTracker
{
public:
	MechanismTracker() : mechanism(nullptr) {}
	~MechanismTracker() { stopTracking(); }

	bool tracking();

	Mechanism* getMechanism();
	void setMechanism(Mechanism* value);

	static void transferTrackers(Mechanism* from, Mechanism* to);

private:
	bool containedBy(Mechanism* value);

	void stopTracking();

	Mechanism* mechanism;
};

class Mechanism
{
public:
	Mechanism() = default;
	Mechanism(const Mechanism&) = delete;
	Mechanism& operator=(const Mechanism&) = delete;

	using AssemblySet = std::set<Assembly*>;

	static Mechanism* getMechanismFromPrimitive(const Primitive* primitive);

	~Mechanism() = default;

	const AssemblySet& getAssemblies() const { return assemblies; }
	AssemblySet& getAssemblies() { return assemblies; }

	void insertAssembly(Assembly* assembly);
	void removeAssembly(Assembly* assembly);

	void absorb(Mechanism* other);

	void notifyMovingPrimitives();

private:
	friend class MechanismTracker;

	AssemblySet assemblies;
	std::vector<MechanismTracker*> trackers;

public:
	std::list<Mechanism*>::iterator myIt;
};

} // namespace RBX

#endif // V8WORLD_MECHANISM_H
