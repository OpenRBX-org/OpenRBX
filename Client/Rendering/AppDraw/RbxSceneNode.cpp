#include "appdraw/RbxSceneNode.h"

#include <OgreMovableObject.h>
#include <OgreRenderQueue.h>
#include <OgreSceneManager.h>
#include <stdexcept>

namespace Ogre {

std::map<const SubMesh*, Sphere>* subMeshSpheres;

void RbxSceneNode::_update(bool updateChildren, bool parentHasChanged)
{
	mDerivedOrientation = mOrientation;
	mDerivedPosition = mPosition;
	mDerivedScale = mScale;
	mWorldAABB.setExtents(-10000.0f, -10000.0f, -10000.0f, 10000.0f, 10000.0f, 10000.0f);
}

void RbxSceneNode::_findVisibleObjects(
	Camera* cam,
	RenderQueue* queue,
	VisibleObjectsBoundsInfo* visibleBounds,
	bool includeChildren,
	bool displayNodes,
	bool onlyShadowCasters
)
{
	ObjectMap::iterator iobj;
	ObjectMap::iterator iobjend = mObjectsByName.end();
	for (iobj = mObjectsByName.begin(); iobj != iobjend; ++iobj) {
		iobj->second->_notifyCurrentCamera(cam);
		if (iobj->second->isVisible() && (!onlyShadowCasters || iobj->second->getCastShadows())) {
			iobj->second->_updateRenderQueue(queue);

			if (visibleBounds) {
				visibleBounds->merge(
					iobj->second->getWorldBoundingBox(true), iobj->second->getWorldBoundingSphere(true), cam
				);
			}
		}
	}

	if (includeChildren) {
		ChildNodeMap::iterator child, childend;
		childend = mChildren.end();
		for (child = mChildren.begin(); child != childend; ++child) {
			SceneNode* sceneChild = static_cast<SceneNode*>(child->second);
			sceneChild->_findVisibleObjects(
				cam, queue, visibleBounds, includeChildren, displayNodes, onlyShadowCasters
			);
		}
	}

	if (displayNodes) {
		queue->addRenderable(this);
	}

	if (mShowBoundingBox || (mCreator && mCreator->getShowBoundingBoxes())) {
		_addBoundingBoxToQueue(queue);
	}
}

void RbxSceneNode::_updateBounds()
{
	mWorldAABB.setNull();

	ObjectMap::iterator i;
	for (i = mObjectsByName.begin(); i != mObjectsByName.end(); ++i) {
		mWorldAABB.merge(i->second->getWorldBoundingBox(true));
	}

	ChildNodeMap::iterator child;
	for (child = mChildren.begin(); child != mChildren.end(); ++child) {
		SceneNode* sceneChild = static_cast<SceneNode*>(child->second);
		mWorldAABB.merge(sceneChild->_getWorldAABB());
	}
}

const Matrix4& RbxSceneNode::getSubMeshTransform(SubMesh* subMesh)
{
	if (subMesh != nullptr) {
		std::map<const SubMesh*, Matrix4*>::iterator it = m_subMeshTransforms.find(subMesh);
		if (it == m_subMeshTransforms.end()) {
			throw std::runtime_error("Transform map error");
		}
		return *it->second;
	}
	if (mCachedTransformOutOfDate) {
		mCachedTransform.makeTransform(_getDerivedPosition(), _getDerivedScale(), _getDerivedOrientation());
		mCachedTransformOutOfDate = false;
	}
	return mCachedTransform;
}

bool RbxSceneNode::hasSubMeshTransform(SubMesh* subMesh) const
{
	std::map<const SubMesh*, Matrix4*>::const_iterator it = m_subMeshTransforms.find(subMesh);
	return it != m_subMeshTransforms.end();
}

void RbxSceneNode::setSubMeshTransform(SubMesh* subMesh, const Matrix4& transform)
{
	const SubMesh* key = subMesh;
	std::map<const SubMesh*, Matrix4*>::iterator it = m_subMeshTransforms.find(key);
	if (it == m_subMeshTransforms.end()) {
		std::pair<std::map<const SubMesh*, Matrix4*>::iterator, bool> inserted;
		inserted = m_subMeshTransforms.insert(std::map<const SubMesh*, Matrix4*>::value_type(key, new Matrix4));
		it = inserted.first;
	}
	*it->second = transform;
}

const Sphere& RbxSceneNode::getSubMeshSphere(SubMesh* subMesh)
{
	std::map<const SubMesh*, Sphere>::const_iterator it = subMeshSpheres->find(subMesh);
	if (it == subMeshSpheres->end()) {
		throw std::runtime_error("bsphere map error");
	}
	return it->second;
}

bool RbxSceneNode::hasSubMeshSphere(SubMesh* subMesh) const
{
	std::map<const SubMesh*, Sphere>::const_iterator it = subMeshSpheres->find(subMesh);
	return it != subMeshSpheres->end();
}

bool RbxSceneNode::removeSubMeshSphere(SubMesh* subMesh)
{
	if (subMeshSpheres == nullptr) {
		return false;
	}
	std::map<const SubMesh*, Sphere>::iterator it = subMeshSpheres->find(subMesh);
	if (it != subMeshSpheres->end()) {
		subMeshSpheres->erase(it);
		return true;
	}
	return false;
}

RbxSceneNode::~RbxSceneNode()
{
	for (std::map<const SubMesh*, Matrix4*>::iterator it = m_subMeshTransforms.begin(); it != m_subMeshTransforms.end();
		 ++it) {
		delete it->second;
	}
	m_subMeshTransforms.clear();
}

void RbxSceneNode::createSubMeshSpheres()
{
	if (subMeshSpheres == nullptr) {
		subMeshSpheres = new std::map<const SubMesh*, Sphere>;
	}
}

void RbxSceneNode::setSubMeshSphere(SubMesh* subMesh, const Sphere& sphere)
{
	createSubMeshSpheres();
	const SubMesh* key = subMesh;
	std::map<const SubMesh*, Sphere>::iterator it = subMeshSpheres->find(key);
	if (it == subMeshSpheres->end()) {
		std::pair<std::map<const SubMesh*, Sphere>::iterator, bool> inserted;
		inserted = subMeshSpheres->insert(std::map<const SubMesh*, Sphere>::value_type(key, Sphere()));
		it = inserted.first;
	}
	it->second = sphere;
}

} // namespace Ogre
