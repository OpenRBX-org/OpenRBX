#ifndef APPDRAW_RBXSCENENODE_H
#define APPDRAW_RBXSCENENODE_H

#include <OgreMatrix4.h>
#include <OgreSceneNode.h>
#include <OgreSphere.h>
#include <map>

namespace Ogre {

class RbxSceneNode : public SceneNode
{
public:
	RbxSceneNode(SceneManager* creator, const String& name) : SceneNode(creator, name) {}

	~RbxSceneNode() override;

	void _update(bool updateChildren, bool parentHasChanged) override;
	void _updateBounds() override;
	void _findVisibleObjects(
		Camera* cam,
		RenderQueue* queue,
		VisibleObjectsBoundsInfo* visibleBounds,
		bool includeChildren = true,
		bool displayNodes = false,
		bool onlyShadowCasters = false
	) override;

	virtual void setSubMeshTransform(SubMesh* subMesh, const Matrix4& transform);
	virtual const Matrix4& getSubMeshTransform(SubMesh* subMesh);
	virtual bool hasSubMeshTransform(SubMesh* subMesh) const;
	virtual void setSubMeshSphere(SubMesh* subMesh, const Sphere& sphere);
	virtual const Sphere& getSubMeshSphere(SubMesh* subMesh);
	virtual bool hasSubMeshSphere(SubMesh* subMesh) const;

	static bool removeSubMeshSphere(SubMesh* subMesh);
	static void createSubMeshSpheres();

private:
	std::map<const SubMesh*, Matrix4*> m_subMeshTransforms;
};

extern std::map<const SubMesh*, Sphere>* subMeshSpheres;

} // namespace Ogre

#endif // APPDRAW_RBXSCENENODE_H
