#include "appdraw/RbxSubEntity.h"

#include "appdraw/VisualEngine.h"

#include <OgreCamera.h>
#include <OgreEntity.h>
#include <OgreHardwareBufferManager.h>
#include <OgreLight.h>
#include <OgreLogManager.h>
#include <OgreSubMesh.h>
#include <exception>
#include <stdexcept>

namespace Ogre {

void RbxSubEntity::setSubMesh(SubMesh* subMesh)
{
	mSubMesh = subMesh;
}

void RbxSubEntity::getRenderOperation(RenderOperation& op)
{
	if (m_lodIndex == 0) {
		mSubMesh->_getRenderOperation(op);
	}
	else {
		m_lodSubMeshes[m_lodIndex - 1]->_getRenderOperation(op);
	}
}

Real RbxSubEntity::getSquaredViewDepth(const Camera* cam) const
{
	if (mCachedCamera == cam) {
		return mCachedCameraDist;
	}
	dynamic_cast<RbxSceneNode*>(mParentEntity->getParentNode());
	Vector3 diff = m_worldTransform.getTrans() - cam->getDerivedPosition();
	return diff.squaredLength();
}

void RbxSubEntity::setMaterial(const MaterialPtr& material)
{
	mpMaterial = material;
}

void RbxSubEntity::setLodSubMesh(int lodIndex, SubMesh* subMesh)
{
	if (lodIndex == 0) {
		mSubMesh = subMesh;
	}
	else {
		m_lodSubMeshes[lodIndex - 1] = subMesh;
	}
}

SubMesh* RbxSubEntity::getLodSubMesh(int lodIndex)
{
	if (lodIndex == 0) {
		return mSubMesh;
	}
	return m_lodSubMeshes[lodIndex - 1];
}

void RbxSubEntity::setLodIndex(int lodIndex)
{
	m_lodIndex = lodIndex;
}

const AxisAlignedBox& RbxSubEntity::getLightCapBounds() const
{
	return getWorldBoundingBox();
}

Real RbxSubEntity::getPointExtrusionDistance(const Light* l) const
{
	return getExtrusionDistance(m_worldTransform.getTrans(), l);
}

EdgeData* RbxSubEntity::getEdgeList()
{
	throw std::runtime_error("RBX RbxSubEntity::getEdgeList(): should not be called");
}

bool RbxSubEntity::hasEdgeList()
{
	throw std::runtime_error("RBX RbxSubEntity::hasEdgeList(): should not be called");
}

RbxSubEntity::RbxSubEntityShadowRenderable::RbxSubEntityShadowRenderable(
	Entity* parent,
	HardwareIndexBufferSharedPtr* indexBuffer,
	const VertexData* vertexData,
	bool createSeparateLightCap,
	RbxSubEntity* subent,
	bool isLightCap
)
	: mSubEntity(subent)
{
	mCurrentVertexData = vertexData;

	mRenderOp.indexData = new IndexData();
	mRenderOp.indexData->indexBuffer = *indexBuffer;
	mRenderOp.indexData->indexStart = 0;

	mRenderOp.vertexData = new VertexData();
	mRenderOp.vertexData->vertexDeclaration->addElement(0, 0, VET_FLOAT3, VES_POSITION);
	mOriginalPosBufferBinding = vertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
	mPositionBuffer = vertexData->vertexBufferBinding->getBuffer(mOriginalPosBufferBinding);
	mRenderOp.vertexData->vertexBufferBinding->setBinding(0, mPositionBuffer);
	if (!vertexData->hardwareShadowVolWBuffer.isNull()) {
		mRenderOp.vertexData->vertexDeclaration->addElement(1, 0, VET_FLOAT1, VES_TEXTURE_COORDINATES, 0);
		mWBuffer = vertexData->hardwareShadowVolWBuffer;
		mRenderOp.vertexData->vertexBufferBinding->setBinding(1, mWBuffer);
	}
	mRenderOp.vertexData->vertexStart = vertexData->vertexStart;

	if (isLightCap) {
		mRenderOp.vertexData->vertexCount = vertexData->vertexCount;
	}
	else {
		mRenderOp.vertexData->vertexCount = vertexData->vertexCount * 2;
		if (createSeparateLightCap) {
			mLightCap = new RbxSubEntityShadowRenderable(parent, indexBuffer, vertexData, false, subent, true);
		}
	}
}

RbxSubEntity::RbxSubEntityShadowRenderable::~RbxSubEntityShadowRenderable()
{
	delete mRenderOp.indexData;
	delete mRenderOp.vertexData;
	mRenderOp.indexData = nullptr;
	mRenderOp.vertexData = nullptr;
}

void RbxSubEntity::RbxSubEntityShadowRenderable::getWorldTransforms(Matrix4* xform) const
{
	mSubEntity->getWorldTransforms(xform);
}

const Quaternion& RbxSubEntity::RbxSubEntityShadowRenderable::getWorldOrientation() const
{
	return mSubEntity->getWorldOrientation();
}

const Vector3& RbxSubEntity::RbxSubEntityShadowRenderable::getWorldPosition() const
{
	return mSubEntity->getWorldPosition();
}

bool RbxSubEntity::RbxSubEntityShadowRenderable::isVisible() const
{
	return mSubEntity->isVisible();
}

const AxisAlignedBox& RbxSubEntity::getWorldBoundingBox(bool derive) const
{
	static AxisAlignedBox box;
	RbxSubMesh* subMesh = m_shadowSubMesh;
	if (!subMesh) {
		for (int i = 0; i >= 0; i--) {
			if (m_lodSubMeshes[i]) {
				subMesh = static_cast<RbxSubMesh*>(m_lodSubMeshes[i]);
				break;
			}
		}
		if (!subMesh) {
			subMesh = static_cast<RbxSubMesh*>(mSubMesh);
		}
	}

	Vector3 center = m_worldTransform.getTrans();
	Real radius = subMesh->m_boundingRadius * 1.7f;
	box.setExtents(center + Vector3(-radius), center + radius);
	return box;
}

const AxisAlignedBox& RbxSubEntity::getDarkCapBounds(const Light& light, Real dirLightExtrusionDist) const
{
	static AxisAlignedBox box;
	box = getLightCapBounds();
	extrudeBounds(box, light.getAs4DVector(), dirLightExtrusionDist);
	return box;
}

RbxSubEntity::RbxSubEntity(Entity* parent) : SubEntity(parent, nullptr), m_lodIndex(0), m_castShadows(true)
{
	m_shadowSubMesh = nullptr;
	m_lodSubMeshes[0] = nullptr;
}

bool RbxSubEntity::getCastShadows() const
{
	return m_castShadows;
}

void RbxSubEntity::getWorldTransforms(Matrix4* xform) const
{
	*xform = m_worldTransform;
}

const Quaternion& RbxSubEntity::getWorldOrientation() const
{
	LogManager::getSingleton().logMessage("RBX perf warning!", LML_NORMAL, false);
	static Quaternion orientation = m_worldTransform.extractQuaternion();
	return orientation;
}

const Vector3& RbxSubEntity::getWorldPosition() const
{
	static Vector3 position(m_worldTransform[0][3], m_worldTransform[1][3], m_worldTransform[2][3]);
	return position;
}

void RbxSubEntity::setRenderImportance(Real value)
{
	m_renderImportance = value;
}

Real RbxSubEntity::getRenderImportance() const
{
	return m_renderImportance;
}

RbxSubEntity::RbxSubEntity(Entity* parent, SubMesh* subMesh, int lodIndex)
	: SubEntity(parent, nullptr), m_lodIndex(lodIndex), m_castShadows(true)
{
	m_lodSubMeshes[0] = nullptr;
	if (lodIndex == 0) {
		mSubMesh = subMesh;
	}
	else {
		m_lodSubMeshes[m_lodIndex - 1] = subMesh;
	}
	m_shadowSubMesh = nullptr;
}

RbxSubEntity::~RbxSubEntity()
{
	for (int i = 0; i < (int) m_shadowRenderables.size(); i++) {
		delete m_shadowRenderables[i];
	}
	m_shadowRenderables.clear();

	if (mSubMesh) {
		if (mSubMesh->useSharedVertices) {
			throw std::runtime_error("RBX RbxSubEntity dtor(1)");
		}
		delete static_cast<RbxSubMesh*>(mSubMesh);
		mSubMesh = nullptr;
	}
	mSubMesh = nullptr;

	for (int i = 0; i < 1; i++) {
		if (m_lodSubMeshes[i]) {
			if (m_lodSubMeshes[i]->useSharedVertices) {
				throw std::runtime_error("RBX RbxSubEntity dtor (2)");
			}
			delete static_cast<RbxSubMesh*>(m_lodSubMeshes[i]);
			m_lodSubMeshes[i] = nullptr;
		}
	}
}

ShadowCaster::ShadowRenderableListIterator RbxSubEntity::getShadowVolumeRenderableIterator(
	ShadowTechnique shadowTechnique,
	const Light* light,
	HardwareIndexBufferSharedPtr* indexBuffer,
	bool extrude,
	Real extrusionDistance,
	unsigned long flags
)
{
	RbxSubMesh* subMesh = m_shadowSubMesh;
	if (!subMesh) {
		for (int i = 0; i >= 0; i--) {
			if (m_lodSubMeshes[i]) {
				subMesh = static_cast<RbxSubMesh*>(m_lodSubMeshes[i]);
				break;
			}
		}
		if (!subMesh) {
			subMesh = static_cast<RbxSubMesh*>(mSubMesh);
		}
	}

	if (m_shadowRenderables.empty()) {
		subMesh->buildEdgeList();
	}

	Vector4 lightPos = light->getAs4DVector();
	Matrix4 world2Obj = m_worldTransform.inverseAffine();
	lightPos = world2Obj.transformAffine(lightPos);

	EdgeData* edgeList = subMesh->m_edgeList;
	if (!edgeList) {
		return ShadowRenderableListIterator(m_shadowRenderables.begin(), m_shadowRenderables.end());
	}

	bool init = m_shadowRenderables.empty();
	EdgeData::EdgeGroupList::iterator egi;
	ShadowRenderableList::iterator si, siend;
	RbxSubEntityShadowRenderable* esr = nullptr;
	if (init) {
		m_shadowRenderables.resize(edgeList->edgeGroups.size());
	}

	siend = m_shadowRenderables.end();
	egi = edgeList->edgeGroups.begin();
	for (si = m_shadowRenderables.begin(); si != siend; ++si, ++egi) {
		const VertexData* pVertData = egi->vertexData;
		if (init) {
			*si = new RbxSubEntityShadowRenderable(nullptr, indexBuffer, pVertData, !extrude, this);
		}

		esr = static_cast<RbxSubEntityShadowRenderable*>(*si);
		HardwareVertexBufferSharedPtr esrPositionBuffer = esr->getPositionBuffer();
		if (extrude) {
			extrudeVertices(esrPositionBuffer, egi->vertexData->vertexCount, lightPos, extrusionDistance);
		}
		esrPositionBuffer->suppressHardwareUpdate(false);
	}

	updateEdgeListLightFacing(edgeList, lightPos);
	generateShadowVolume(edgeList, *indexBuffer, light, m_shadowRenderables, flags);

	return ShadowRenderableListIterator(m_shadowRenderables.begin(), m_shadowRenderables.end());
}

} // namespace Ogre
