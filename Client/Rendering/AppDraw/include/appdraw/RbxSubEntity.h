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

class Unknown00671c70;

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

	void FUN_0067b3d0(SubMesh* subMesh);
	void FUN_0067b4b0(const MaterialPtr& material);
	void FUN_0067b4c0(int lodIndex, SubMesh* subMesh);
	SubMesh* FUN_0067b4e0(int lodIndex);
	void FUN_0067b500(int lodIndex);

	void getRenderOperation(RenderOperation& op) override;
	void getWorldTransforms(Matrix4* xform) const override;
	const Quaternion& getWorldOrientation() const override;
	const Vector3& getWorldPosition() const override;
	Real getSquaredViewDepth(const Camera* cam) const override;

	virtual void FUN_0067bcc0(Real value);
	virtual Real FUN_0067bcd0() const;

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

	Matrix4 m_unk0xdc;
	SubMesh* m_unk0x11c[1];
	int m_unk0x120;
	Real m_unk0x124;
	bool m_unk0x128;
	Unknown00671c70* m_unk0x12c;
	std::vector<ShadowRenderable*> m_unk0x130;
};

} // namespace Ogre

#endif // APPDRAW_RBXSUBENTITY_H
