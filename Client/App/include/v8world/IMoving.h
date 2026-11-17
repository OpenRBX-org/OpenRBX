#ifndef V8WORLD_IMOVING_H
#define V8WORLD_IMOVING_H

#include <set>

namespace RBX {

class IMoving;

class IMovingManager
{
public:
	using MovingSet = std::set<IMoving*>;

	IMovingManager();

	virtual ~IMovingManager();

	void onMovingHeartbeat();

protected:
	void moved(IMoving* m) { moving.insert(m); }

	void remove(IMoving* iMoving);

private:
	MovingSet moving;
	MovingSet::iterator current;

	friend class IMoving;
};

struct CanAggregateChanged
{
	const bool canClump;

	CanAggregateChanged(const CanAggregateChanged& other) = default;
	CanAggregateChanged(bool canClump) : canClump(canClump) {}
};

class IMoving
{
protected:
	virtual void onCanAggregateChanged(bool value) = 0;

public:
	IMoving();

	virtual bool reportTouches() const = 0;

	void notifyMoved();

	bool getCanAggregate() const { return stepsToSleep == 0; }

protected:
	bool checkSleep();

	void setMovingManager(IMovingManager* value);

private:
	void makeMoving();

	IMovingManager* iMovingManager;
	int stepsToSleep;

	friend class IMovingManager;
};

} // namespace RBX

#endif // V8WORLD_IMOVING_H
