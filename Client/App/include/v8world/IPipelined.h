#ifndef V8WORLD_IPIPELINED_H
#define V8WORLD_IPIPELINED_H

#include "v8kernel/IStage.h"
#include "v8world/IWorldStage.h"

#include <cstddef>

namespace RBX {

class Kernel;
class World;

class IPipelined
{
private:
	IStage* currentStage;

public:
	void removeFromStage(IStage* stage);

private:
	void removeFromStage(IStage::StageType stageType);
	IStage* getStage(IStage::StageType stageType) const;

public:
	IPipelined() : currentStage(nullptr) {}

	virtual ~IPipelined() = default;

	void putInPipeline(IStage* stage);
	void removeFromPipeline(IStage* stage);
	void putInStage(IStage* stage);

	bool inPipeline() const { return currentStage != nullptr; }
	bool inStage(IStage* stage) const { return currentStage == stage; }
	bool inStage(IStage::StageType stageType) const;
	bool inOrDownstreamOfStage(IStage* stage) const;
	bool downstreamOfStage(IStage* stage) const;
	bool inKernel() const { return currentStage->getStageType() == IStage::KERNEL_STAGE; }

	Kernel* getKernel() const;

	virtual void putInKernel(Kernel* kernel);
	virtual void removeFromKernel();

	World* getWorld();
};

inline World* IPipelined::getWorld()
{
	if (currentStage == nullptr) {
		return nullptr;
	}

	IStage* stage;

	if (currentStage->getStageType() != IStage::KERNEL_STAGE) {
		stage = currentStage;
	}
	else {
		stage = currentStage->getUpstream();
	}

	return static_cast<IWorldStage*>(stage)->getWorld();
}

inline bool IPipelined::downstreamOfStage(IStage* iStage) const
{
	return currentStage->getStageType() > iStage->getStageType();
}

inline bool IPipelined::inOrDownstreamOfStage(IStage* stage) const
{
	return currentStage->getStageType() >= stage->getStageType();
}

} // namespace RBX

#endif // V8WORLD_IPIPELINED_H
