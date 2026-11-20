#ifndef APPDRAW_RBXSUBENTITY_H
#define APPDRAW_RBXSUBENTITY_H

#include "appdraw/RbxSceneNode.h"

#include <OgreMatrix4.h>
#include <OgreSceneNode.h>
#include <OgreShadowCaster.h>
#include <OgreSubEntity.h>
#include <vector>

namespace RBX {
class AdornRbxGfx;
namespace RenderNew {
class RenderScene;
} // namespace RenderNew
} // namespace RBX

namespace Ogre {

class RbxSubMesh;

class RbxSubEntity : public SubEntity, public ShadowCaster
{
public:
	class RbxSubEntityShadowRenderable : public ShadowRenderable
	{
	protected:
		HardwareVertexBufferSharedPtr mPositionBuffer;
		HardwareVertexBufferSharedPtr mWBuffer;
		const VertexData* mCurrentVertexData;
		unsigned short mOriginalPosBufferBinding;
		RbxSubEntity* mSubEntity;

	public:
		RbxSubEntityShadowRenderable(
			Entity* parent,
			HardwareIndexBufferSharedPtr* indexBuffer,
			const VertexData* vertexData,
			bool createSeparateLightCap,
			RbxSubEntity* subent,
			bool isLightCap = false
		);
		~RbxSubEntityShadowRenderable() override;

		HardwareVertexBufferSharedPtr getPositionBuffer() { return mPositionBuffer; }

		void getWorldTransforms(Matrix4* xform) const override;
		const Quaternion& getWorldOrientation() const override;
		const Vector3& getWorldPosition() const override;
		bool isVisible() const override;
	};

	RbxSubEntity(Entity* parent);
	RbxSubEntity(Entity* parent, SubMesh* subMesh, int lodIndex);
	~RbxSubEntity() override;

	void setSubMesh(SubMesh* subMesh);
	void setMaterial(const MaterialPtr& material);
	void setLodSubMesh(int lodIndex, SubMesh* subMesh);
	SubMesh* getLodSubMesh(int lodIndex);
	void setLodIndex(int lodIndex);

	void getRenderOperation(RenderOperation& op) override;
	void getWorldTransforms(Matrix4* xform) const override;
	const Quaternion& getWorldOrientation() const override;
	const Vector3& getWorldPosition() const override;
	Real getSquaredViewDepth(const Camera* cam) const override;

	virtual void setRenderImportance(Real value);
	virtual Real getRenderImportance() const;

	bool getCastShadows() const override;
	EdgeData* getEdgeList() override;
	bool hasEdgeList() override;
	const AxisAlignedBox& getWorldBoundingBox(bool derive = false) const override;
	const AxisAlignedBox& getLightCapBounds() const override;
	const AxisAlignedBox& getDarkCapBounds(const Light& light, Real dirLightExtrusionDist) const override;
	ShadowRenderableListIterator getShadowVolumeRenderableIterator(
		ShadowTechnique shadowTechnique,
		const Light* light,
		HardwareIndexBufferSharedPtr* indexBuffer,
		bool extrudeVertices,
		Real extrusionDistance,
		unsigned long flags = 0
	) override;
	Real getPointExtrusionDistance(const Light* l) const override;

private:
	friend class RBX::RenderNew::RenderScene;
	friend class RbxEntity;
	friend class RBX::AdornRbxGfx;

	Matrix4 m_worldTransform;
	SubMesh* m_lodSubMeshes[1];
	int m_lodIndex;
	Real m_renderImportance;
	bool m_castShadows;
	RbxSubMesh* m_shadowSubMesh;
	std::vector<ShadowRenderable*> m_shadowRenderables;
};

} // namespace Ogre

#endif // APPDRAW_RBXSUBENTITY_H
