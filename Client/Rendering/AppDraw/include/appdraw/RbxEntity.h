#ifndef APPDRAW_RBXENTITY_H
#define APPDRAW_RBXENTITY_H

#include "appdraw/VisualEngine.h"

#include <OgreAxisAlignedBox.h>
#include <OgreEntity.h>
#include <vector>

namespace RBX::RenderNew {
class RenderScene;
} // namespace RBX::RenderNew

namespace Ogre {

class RbxSubEntity;

class RbxEntity : public Entity
{
public:
	struct Candidate
	{
		bool operator<(const Candidate& other) const { return m_priority < other.m_priority; }

		int m_index;
		Real m_priority;
	};

	RbxEntity();
	~RbxEntity() override;

	void _notifyCurrentCamera(Camera* cam) override;
	void _updateRenderQueue(RenderQueue* queue) override;

	void setBounds(AxisAlignedBox box) { m_bounds = box; }

	void setFrameRateManager(VisualEngine::FrameRateManager* frameRateManager);
	void setMesh(MeshPtr mesh);
	void removeSubEntity(RbxSubEntity* subEntity);
	void clearSubEntities();
	RbxSubEntity* createSubEntity(SubMesh* subMesh, int lodIndex);
	void addSubEntity(RbxSubEntity* subEntity);
	void findShadowCasters(Camera* camera, const Light* light, std::vector<ShadowCaster*>* casters);

	virtual void updateVisibility(Camera* cam);
	virtual void setSubEntitiesVisible(bool visible);

private:
	friend class RBX::AdornRbxGfx;
	friend class RBX::RenderNew::RenderScene;

	VisualEngine::FrameRateManager* m_frameRateManager;
	bool m_frustumCull;
	bool m_visibilityUpdated;
	bool m_recordFrameStats;
	AxisAlignedBox m_bounds;
	Real m_minVisiblePercent;
	int m_minVisibleCount;
	std::vector<unsigned char> m_subEntityStates;
};

class RbxEntityFactory : public MovableObjectFactory
{
public:
	static String FACTORY_TYPE_NAME;
};

} // namespace Ogre

#endif // APPDRAW_RBXENTITY_H
