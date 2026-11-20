#ifndef APPDRAW_ADORNRBXGFX_H
#define APPDRAW_ADORNRBXGFX_H

#include "rbxgraphics/Adorn.h"

#include <G3D/CoordinateFrame.h>
#include <OgreMaterial.h>
#include <OgreMesh.h>
#include <map>
#include <string>
#include <vector>

namespace Ogre {
class RbxEntity;
class RbxSceneNode;
class RbxSubEntity;
class SceneNode;
class VisualEngine;
} // namespace Ogre

namespace RBX {

class AdornRbxGfx : public Adorn
{
private:
	Ogre::VisualEngine* m_visualEngine;
	std::string m_textureName;
	Ogre::MaterialPtr m_material;
	Ogre::SceneNode* m_rootNode;
	Ogre::RbxSceneNode* m_meshNode;
	Ogre::MaterialPtr m_unlitMaterial;
	std::vector<Ogre::RbxSubEntity*> m_freeSubEntities;
	std::vector<Ogre::RbxSubEntity*> m_usedSubEntities;
	std::map<std::string, Ogre::MeshPtr> m_meshes;
	Ogre::RbxEntity* m_entity;
	Ogre::SceneNode* m_explosionRoot;
	std::vector<Ogre::SceneNode*> m_explosionNodes;
	std::map<std::string, Ogre::SceneNode*> m_activeExplosions;
	G3D::CoordinateFrame m_objectToWorld;

	void makeTransform(
		Ogre::Matrix4& transform,
		const Ogre::Vector3& offset,
		const Ogre::Vector3& scale,
		const Ogre::Quaternion& rotation
	);
	void makeObjectTransform(Ogre::Matrix4& transform);
	std::string nextMeshName();
	void setColor(const G3D::Color4& color, int variant);
	Ogre::MeshPtr findMesh(std::string name);
	void addMesh(const std::string& name, const Ogre::MeshPtr& mesh);
	Ogre::RbxSubEntity* allocateSubEntity();
	void releaseSubEntities();
	void addMeshInstance(const Ogre::MeshPtr& mesh, const Ogre::Matrix4& transform, const Ogre::MaterialPtr& material);

public:
	AdornRbxGfx(Ogre::VisualEngine* visualEngine);
	~AdornRbxGfx() override;

	G3D::ReferenceCountedPointer<TextureProxyBase> createTextureProxy(const std::string& name) override;
	void prepareRenderPass() override;
	void finishRenderPass() override;
	G3D::Rect2D getViewport() const override;
	int getWidth() const override;
	int getHeight() const override;
	void setTexture(int stage, G3D::ReferenceCountedPointer<TextureProxyBase> texture) override;
	G3D::Rect2D getTextureSize(G3D::ReferenceCountedPointer<TextureProxyBase> texture) const override;
	void line2d(const G3D::Vector2& from, const G3D::Vector2& to, const G3D::Color4& color) const override;
	void outlineRect2d(const G3D::Rect2D& rect, float thickness, const G3D::Color4& color) const override;
	void rect2d(const G3D::Rect2D& rect, const G3D::Color4& color) const override;
	G3D::Vector2 get2DStringBounds(const std::string& s, double size, Spacing spacing) const override;
	G3D::Vector2 drawFont2D(
		const std::string& s,
		const G3D::Vector2& position,
		double size,
		const G3D::Color4& color,
		const G3D::Color4& outline,
		XAlign xAlign,
		YAlign yAlign,
		Spacing spacing
	) const override;
	void setObjectToWorldMatrix(const G3D::CoordinateFrame& coord) override;
	void setColor(const G3D::Color4& color) override {}
	void box(const G3D::AABox& box, const G3D::Color4& solid, const G3D::Color4& wire) override;
	void sphere(const G3D::Sphere& sphere, const G3D::Color4& solid, const G3D::Color4& wire) override;
	void explosion(const G3D::Sphere& sphere, const G3D::Color4& solid, const G3D::Color4& wire) override;
	void cylinderAlongX(float radius, float length, const G3D::Color4& solid, const G3D::Color4& wire) override;
	void ray(const G3D::Ray& ray, const G3D::Color4& color, float scale) override;
	void lineSegment(const G3D::LineSegment& segment, const G3D::Color4& color, float scale) override;
	void axes(const G3D::Color4& x, const G3D::Color4& y, const G3D::Color4& z, float scale) override;
	void quad(
		const G3D::Vector3& v0,
		const G3D::Vector3& v1,
		const G3D::Vector3& v2,
		const G3D::Vector3& v3,
		const G3D::Color4& color
	) override;
};

} // namespace RBX

#endif // APPDRAW_ADORNRBXGFX_H
