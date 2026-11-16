#ifndef V8DATAMODEL_JOINTSSERVICE_H
#define V8DATAMODEL_JOINTSSERVICE_H

#include "util/Events.h"
#include "util/IRenderable.h"
#include "v8datamodel/JointInstance.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"
#include "v8world/World.h"

#include <memory>

namespace RBX {

class JointsService : public NonFactoryProduct<Instance, nullptr>,
					  public Service,
					  public IRenderableBucket,
					  public Listener<World, AutoJoin>,
					  public Listener<World, AutoDestroy>
{
private:
	void onDescendentAdded(Instance* instance) override;
	void onDescendentRemoving(const std::shared_ptr<Instance>& instance) override;

	bool askAddChild(const Instance* instance) const override
	{
		return dynamic_cast<const JointInstance*>(instance) != nullptr;
	}
	XmlElement* write() override { return nullptr; }
	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;
	void onEvent(const World* source, AutoDestroy event) override;
	void onEvent(const World* source, AutoJoin event) override;

public:
	World* world;

	JointsService() : world(nullptr) { setName("JointsService"); }
};

} // namespace RBX

#endif // V8DATAMODEL_JOINTSSERVICE_H
