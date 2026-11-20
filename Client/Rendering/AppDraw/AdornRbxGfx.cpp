#include "appdraw/AdornRbxGfx.h"

#include "appdraw/Quadric.h"
#include "appdraw/RbxEntity.h"
#include "appdraw/RbxFont.h"
#include "appdraw/RbxSceneNode.h"
#include "appdraw/RbxSubEntity.h"
#include "appdraw/VisualEngine.h"
#include "renderlibnew/TextureProxy.h"
#include "util/ContentProvider.h"
#include "util/IndexBox.h"
#include "util/Utilities.h"

#include <OgreLogManager.h>
#include <OgreManualObject.h>
#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgreParticleEmitter.h>
#include <OgreParticleSystem.h>
#include <OgreParticleSystemManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreStringConverter.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <OgreViewport.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace RBX {

Ogre::SharedPtr<Ogre::RbxFont> adornFont;

int meshNameCounter;

std::string sphereMeshSuffix = "q0adorn";

std::string explosionMaterialScript = "material explosion/explosionMatl\n"
									  "{\n"
									  "\ttechnique\n"
									  "\t{\n"
									  "\t\tpass\n"
									  "\t\t{\n"
									  "\t\t\tlighting off\n"
									  "\t\t\tdepth_write off\n"
									  "\t\t\tscene_blend add\n"
									  "\n"
									  "\t\t\ttexture_unit\n"
									  "\t\t\t{\n"
									  "\t\t\t\ttexture explosion.png\n"
									  "\t\t\t\tcolour_op modulate\n"
									  "\t\t\t}\n"
									  "\t\t}\n"
									  "\t}\n"
									  "}\n";

std::string explosionParticleScript = "explosion/explosion1\n"
									  "{\n"
									  "\tquota\t400\n"
									  "\tmaterial\texplosion/explosionMatl\n"
									  "\tparticle_width\t5\n"
									  "\tparticle_height\t5\n"
									  "\tcull_each\tfalse\n"
									  "\trenderer\tbillboard\n"
									  "\tsorted\tfalse\n"
									  "\tlocal_space\tfalse\n"
									  "\titeration_interval\t0\n"
									  "\tnonvisible_update_timeout\t0\n"
									  "\tbillboard_type\tpoint\n"
									  "\tbillboard_origin\tcenter\n"
									  "\tbillboard_rotation_type\tvertex\n"
									  "\tcommon_up_vector\t0 1 0\n"
									  "\tpoint_rendering\tfalse\n"
									  "\taccurate_facing\tfalse\n"
									  "\n"
									  "\temitter Point\n"
									  "\t{\n"
									  "\t\tangle\t180\n"
									  "\t\tcolour\t0.3 0.3 0.3 .6\n"
									  "\t\tcolour_range_start\t0.25 0.25 0.25 1\n"
									  "\t\tcolour_range_end\t0.5 0.5 0.5 1\n"
									  "\t\tdirection\t0 -1 0\n"
									  "\t\temission_rate\t20000\n"
									  "\t\tposition\t0 0 0\n"
									  "\t\tvelocity\t30\n"
									  "\t\tvelocity_min\t20\n"
									  "\t\tvelocity_max\t30\n"
									  "\t\ttime_to_live\t0.3\n"
									  "\t\ttime_to_live_min\t0.25\n"
									  "\t\ttime_to_live_max\t0.35\n"
									  "\t\tduration\t1.2\n"
									  "\t\tduration_min\t1.2\n"
									  "\t\tduration_max\t1.2\n"
									  "\t\trepeat_delay\t10000\n"
									  "\t\trepeat_delay_min\t10000\n"
									  "\t\trepeat_delay_max\t10000\n"
									  "\t}\n"
									  "\n"
									  "\taffector DeflectorPlane\n"
									  "\t{\n"
									  "\t\tplane_point\t0 0 0\n"
									  "\t\tplane_normal\t0 1 0\n"
									  "\t\tbounce\t0\n"
									  "\t}\n"
									  "\n"
									  "\taffector DirectionRandomiser\n"
									  "\t{\n"
									  "\t\trandomness\t0\n"
									  "\t\tscope\t1\n"
									  "\t\tkeep_velocity\tfalse\n"
									  "\t}\n"
									  "}\n";

std::string cylinderMeshSuffix = "c01adorn";

Ogre::MeshPtr cylinderMesh;

Ogre::MeshPtr sphereMesh;

int adornCount;

int AdornRbxGfx::getWidth() const
{
	return m_visualEngine->getViewport()->getActualWidth();
}

int AdornRbxGfx::getHeight() const
{
	return m_visualEngine->getViewport()->getActualHeight();
}

void AdornRbxGfx::rect2d(const G3D::Rect2D& rect, const G3D::Color4& color) const
{
	m_visualEngine->m_twoDManager->spriteBltFull(
		m_textureName,
		color,
		rect.x0(),
		getHeight() - 1 - rect.y0(),
		rect.x1(),
		getHeight() - 1 - rect.y1(),
		0.0f,
		0.0f,
		1.0f,
		1.0f
	);
}

void AdornRbxGfx::outlineRect2d(const G3D::Rect2D& rect, float thickness, const G3D::Color4& color) const
{
	float y1 = rect.y1();
	float x1 = rect.x1();
	float y0 = rect.y0();
	float x0 = rect.x0();
	Ogre::TwoDManager* twoDManager = m_visualEngine->m_twoDManager;
	twoDManager->rectBlt(2, x0, getHeight() - 1 - y0, x1, getHeight() - 1 - y1, color);
}

void AdornRbxGfx::line2d(const G3D::Vector2& from, const G3D::Vector2& to, const G3D::Color4& color) const
{
	Ogre::TwoDManager* twoDManager = m_visualEngine->m_twoDManager;
	twoDManager->lineBlt(from.x, getHeight() - 1 - from.y, to.x, getHeight() - 1 - to.y, color);
}

void AdornRbxGfx::makeTransform(
	Ogre::Matrix4& transform, const Ogre::Vector3& offset, const Ogre::Vector3& scale, const Ogre::Quaternion& rotation
)
{
	Ogre::Quaternion orientation;
	orientation.FromRotationMatrix(reinterpret_cast<const Ogre::Matrix3&>(m_objectToWorld.rotation));
	orientation = orientation * rotation;
	transform.makeTransform(
		reinterpret_cast<const Ogre::Vector3&>(m_objectToWorld.translation) + offset, scale, orientation
	);
}

void AdornRbxGfx::makeObjectTransform(Ogre::Matrix4& transform)
{
	Ogre::Quaternion orientation;
	orientation.FromRotationMatrix(reinterpret_cast<const Ogre::Matrix3&>(m_objectToWorld.rotation));
	transform.makeTransform(
		reinterpret_cast<const Ogre::Vector3&>(m_objectToWorld.translation),
		Ogre::Vector3(1.0f, 1.0f, 1.0f),
		orientation
	);
}

std::string AdornRbxGfx::nextMeshName()
{
	int id = 0;
	id = meshNameCounter++;
	return StringConverter<int>::convertToString(id);
}

G3D::Vector2 AdornRbxGfx::drawFont2D(
	const std::string& s,
	const G3D::Vector2& position,
	double size,
	const G3D::Color4& color,
	const G3D::Color4& outline,
	XAlign xAlign,
	YAlign yAlign,
	Spacing spacing
) const
{
	float x = position.x;
	float y = getHeight() - 1 - position.y;
	Ogre::Vector2 pos(x, y);
	Ogre::Vector2 extent = adornFont->draw2D(m_visualEngine, s, pos, size, color, outline, xAlign, yAlign, spacing);
	return G3D::Vector2(extent.x, extent.y);
}

G3D::Vector2 AdornRbxGfx::get2DStringBounds(const std::string& s, double size, Spacing spacing) const
{
	Ogre::Vector2 bounds = adornFont->get2DStringBounds(s, size, spacing);
	return G3D::Vector2(bounds.x, bounds.y);
}

void AdornRbxGfx::setObjectToWorldMatrix(const G3D::CoordinateFrame& coord)
{
	m_objectToWorld = coord;
}

G3D::Rect2D AdornRbxGfx::getViewport() const
{
	Ogre::Viewport* viewport = m_visualEngine->getViewport();
	float left = viewport->getActualLeft();
	float top = viewport->getActualTop();
	float width = viewport->getActualWidth();
	float height = viewport->getActualHeight();
	return G3D::Rect2D::xywh(left, top, width, height);
}

void AdornRbxGfx::setTexture(int stage, G3D::ReferenceCountedPointer<TextureProxyBase> texture)
{
	if (texture.isNull()) {
		m_textureName = "";
		return;
	}

	RenderNew::TextureProxy* proxy = static_cast<RenderNew::TextureProxy*>(texture.pointer());
	m_textureName = proxy->filename;
	proxy->resolve(reinterpret_cast<G3D::RenderDevice*>(m_visualEngine));
}

G3D::Rect2D AdornRbxGfx::getTextureSize(G3D::ReferenceCountedPointer<TextureProxyBase> texture) const
{
	RenderNew::TextureProxy* proxy = static_cast<RenderNew::TextureProxy*>(texture.pointer());
	proxy->resolve(reinterpret_cast<G3D::RenderDevice*>(m_visualEngine));

	Ogre::TexturePtr ogreTexture = Ogre::TextureManager::getSingleton().getByName(proxy->filename);
	size_t width = ogreTexture->getWidth();
	size_t height = ogreTexture->getHeight();
	return G3D::Rect2D(G3D::Vector2((float) width, (float) height));
}

G3D::ReferenceCountedPointer<TextureProxyBase> AdornRbxGfx::createTextureProxy(const std::string& name)
{
	return new RenderNew::TextureProxy(name);
}

void AdornRbxGfx::setColor(const G3D::Color4& color, int variant)
{
	char name[64];
	snprintf(name, sizeof(name), "setColor%.3f%.3f%.3f%.3f%d", color.r, color.g, color.b, color.a, variant);
	m_material = Ogre::MaterialManager::getSingleton().getByName(name);
	if (m_material.isNull()) {
		m_material = Ogre::MaterialManager::getSingleton().create(name, "adorn3d");
		Ogre::Pass* pass = m_material->getTechnique(0)->getPass(0);
		pass->setAmbient(color.r, color.g, color.b);
		pass->setDiffuse(color.r, color.g, color.b, color.a);
		pass->setSpecular(color.r, color.g, color.b, color.a);
		pass->setShininess(10.0f);
		m_material->setLightingEnabled(true);
	}
}

Ogre::MeshPtr AdornRbxGfx::findMesh(std::string name)
{
	std::map<std::string, Ogre::MeshPtr>::const_iterator it = m_meshes.find(name);
	if (it != m_meshes.end()) {
		return it->second;
	}

	return Ogre::MeshPtr();
}

void AdornRbxGfx::prepareRenderPass()
{
	int total = 0;
	std::map<std::string, Ogre::SceneNode*>::iterator it;
	for (it = m_activeExplosions.begin(); it != m_activeExplosions.end(); ++it) {
		Ogre::ParticleSystem* particleSystem =
			static_cast<Ogre::ParticleSystem*>(it->second->getAttachedObject((unsigned short) 0));
		total += particleSystem->getParticleQuota();
	}

	float quota = total;
	if (quota > 11000.0f) {
		float scale = 10000.0 / quota;
		for (it = m_activeExplosions.begin(); it != m_activeExplosions.end(); ++it) {
			Ogre::ParticleSystem* particleSystem =
				static_cast<Ogre::ParticleSystem*>(it->second->getAttachedObject((unsigned short) 0));
			Ogre::ParticleEmitter* emitter = particleSystem->getEmitter(0);
			int particleQuota = particleSystem->getParticleQuota();
			float minVelocity = emitter->getMinParticleVelocity();
			float maxVelocity = emitter->getMaxParticleVelocity();
			float duration = emitter->getDuration();
			particleSystem->setParticleQuota((size_t) (particleQuota * scale));
			emitter->setMinParticleVelocity(pow(scale, 1.0f / 3.0f) * minVelocity);
			emitter->setMaxParticleVelocity(pow(scale, 1.0f / 3.0f) * maxVelocity);
			emitter->setMinDuration((scale - 1.0) * 0.4f + duration);
			emitter->setMaxDuration((scale - 1.0) * 0.4f + duration);
		}
	}
}

void AdornRbxGfx::addMesh(const std::string& name, const Ogre::MeshPtr& mesh)
{
	m_meshes[name] = mesh;
	Ogre::Sphere sphere(mesh->getBounds().getCenter(), mesh->getBoundingSphereRadius());
	for (int i = 0; i < mesh->getNumSubMeshes(); i++) {
		m_meshNode->setSubMeshSphere(mesh->getSubMesh(i), sphere);
	}
}

Ogre::RbxSubEntity* AdornRbxGfx::allocateSubEntity()
{
	if (m_freeSubEntities.size() != 0) {
		Ogre::RbxSubEntity* subEntity = m_freeSubEntities[m_freeSubEntities.size() - 1];
		m_freeSubEntities.pop_back();
		m_usedSubEntities.push_back(subEntity);
		return subEntity;
	}

	Ogre::RbxSubEntity* subEntity = new Ogre::RbxSubEntity(m_entity);
	m_usedSubEntities.push_back(subEntity);
	return subEntity;
}

void AdornRbxGfx::releaseSubEntities()
{
	for (int i = 0; i < (int) m_usedSubEntities.size(); i++) {
		m_freeSubEntities.push_back(m_usedSubEntities[i]);
	}
	m_usedSubEntities.clear();
}

void AdornRbxGfx::addMeshInstance(
	const Ogre::MeshPtr& mesh, const Ogre::Matrix4& transform, const Ogre::MaterialPtr& material
)
{
	for (int i = 0; i < mesh->getNumSubMeshes(); i++) {
		Ogre::RbxSubEntity* subEntity = allocateSubEntity();
		subEntity->setSubMesh(mesh->getSubMesh(i));
		subEntity->m_worldTransform = transform;
		subEntity->setMaterial(material);
		m_entity->addSubEntity(subEntity);
	}
}

void AdornRbxGfx::finishRenderPass()
{
	m_rootNode->removeAndDestroyAllChildren();
	m_visualEngine->m_sceneManager->destroyAllManualObjects();
	meshNameCounter = 0;
	m_entity->clearSubEntities();
	releaseSubEntities();
}

void AdornRbxGfx::box(const G3D::AABox& box, const G3D::Color4& solid, const G3D::Color4& wire)
{
	IndexBox indexBox(G3D::Vector3(-1.0f, -1.0f, -1.0f), G3D::Vector3(1.0f, 1.0f, 1.0f));

	char key[64];
	snprintf(key, sizeof(key), "c%d%d%d", (int) (solid.r * 255.0), (int) (solid.g * 255.0), (int) (solid.b * 255.0));
	Ogre::MeshPtr mesh = findMesh(key);
	if (mesh.isNull()) {
		Ogre::ManualObject* manual = m_visualEngine->getSceneManager()->createManualObject("box" + nextMeshName());
		manual->begin("", Ogre::RenderOperation::OT_TRIANGLE_LIST);
		manual->colour(Ogre::toOgreColour(solid));

		int index = 0;
		for (int face = 0; face < 6; face++) {
			G3D::Vector3 v0;
			G3D::Vector3 v1;
			G3D::Vector3 v2;
			G3D::Vector3 v3;
			indexBox.getFaceCorners(face, v0, v1, v2, v3);
			manual->normal(Ogre::toOgreVector3(indexBox.getFaceNormal(face)));
			manual->position(Ogre::toOgreVector3(v0));
			manual->index(index++);
			manual->position(Ogre::toOgreVector3(v1));
			manual->index(index++);
			manual->position(Ogre::toOgreVector3(v2));
			manual->index(index++);
			manual->position(Ogre::toOgreVector3(v0));
			manual->index(index++);
			manual->position(Ogre::toOgreVector3(v2));
			manual->index(index++);
			manual->position(Ogre::toOgreVector3(v3));
			manual->index(index++);
		}

		manual->end();
		manual->setMaterialName(0, m_unlitMaterial->getName());
		mesh = manual->convertToMesh(
			m_visualEngine->m_sceneManager->getName() + "bmsh" + Ogre::StringConverter::toString(m_meshes.size()),
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
		);
		addMesh(std::string(key), mesh);
		Ogre::Sphere bounds(mesh->getBounds().getCenter(), mesh->getBoundingSphereRadius());
		m_meshNode->setSubMeshSphere(mesh->getSubMesh(0), bounds);
	}

	Ogre::Matrix4 local;
	Ogre::Matrix4 transform;
	makeTransform(transform, Ogre::Vector3(0.0f, 0.0f, 0.0f), Ogre::Vector3(1.0f, 1.0f, 1.0f), Ogre::Quaternion());
	local.makeTransform(
		Ogre::toOgreVector3(box.center()),
		Ogre::Vector3(
			(box.high().x - box.low().x) * 0.5, (box.high().y - box.low().y) * 0.5, (box.high().z - box.low().z) * 0.5
		),
		Ogre::Quaternion()
	);

	addMeshInstance(mesh, transform * local, m_unlitMaterial);
}

void AdornRbxGfx::cylinderAlongX(float radius, float length, const G3D::Color4& solid, const G3D::Color4& wire)
{
	Ogre::Sphere bounds(
		Ogre::Vector3(m_objectToWorld.translation + G3D::Vector3(length * 0.5, 0.0f, 0.0f)),
		radius > length ? radius * 1.4f : length * 1.4f
	);
	if (!m_visualEngine->m_camera->isVisible(bounds, nullptr)) {
		return;
	}

	setColor(solid, 0);
	Ogre::Quadric quadric;
	if (cylinderMesh.isNull()) {
		Ogre::ManualObject* manual = quadric.cylinder(
			m_visualEngine->getSceneManager(), "cyl" + nextMeshName(), nullptr, 1.0f, 1.0f, 1.0f, 12, 1
		);
		quadric.setOffset(Ogre::Vector3(0.0f, 0.0f, 1.0f));
		quadric.disk(m_visualEngine->getSceneManager(), "dsk" + nextMeshName(), manual, 0.0f, 1.0f, 12, 1);
		quadric.setOffset(Ogre::Vector3(0.0f, 0.0f, 0.0f));
		quadric.orientation = 1;
		quadric.disk(m_visualEngine->getSceneManager(), "dsk" + nextMeshName(), manual, 0.0f, 1.0f, 12, 1);
		manual->setMaterialName(0, m_material->getName());
		manual->setMaterialName(1, m_material->getName());
		manual->setMaterialName(2, m_material->getName());
		cylinderMesh = manual->convertToMesh(
			m_visualEngine->getSceneManager()->getName() + cylinderMeshSuffix,
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
		);
		Ogre::Sphere meshBounds(cylinderMesh->getBounds().getCenter(), cylinderMesh->getBoundingSphereRadius());
		for (int i = 0; i < cylinderMesh->getNumSubMeshes(); i++) {
			m_meshNode->setSubMeshSphere(cylinderMesh->getSubMesh(i), meshBounds);
		}
	}

	Ogre::Matrix4 world;
	makeObjectTransform(world);

	Ogre::Matrix4 local;
	local.makeTransform(
		Ogre::Vector3(-length * 0.5, 0.0f, 0.0f),
		Ogre::Vector3(radius, radius, length),
		Ogre::Quaternion(Ogre::Radian(Ogre::Degree(90.0f)), Ogre::Vector3(0.0f, 1.0f, 0.0f))
	);

	addMeshInstance(cylinderMesh, world * local, m_material);
}

void AdornRbxGfx::sphere(const G3D::Sphere& sphere, const G3D::Color4& solid, const G3D::Color4& wire)
{
	Ogre::Quadric quadric;
	setColor(G3D::Color4(0.8f * solid.r, 0.8f * solid.g, 0.8f * solid.b, 0.8f * solid.a), 0);

	std::string key("sph0001");
	Ogre::MeshPtr mesh = findMesh(key);
	if (mesh.isNull()) {
		Ogre::ManualObject* manual =
			quadric.sphere(m_visualEngine->getSceneManager(), "sph" + nextMeshName(), nullptr, 1.0f, 17, 17);
		for (int i = 0; i < (int) manual->getNumSections(); i++) {
			manual->setMaterialName(i, m_material->getName());
		}
		mesh = manual->convertToMesh(
			m_visualEngine->m_sceneManager->getName() + "smsh" + Ogre::StringConverter::toString(m_meshes.size()),
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
		);
		m_meshes[key] = mesh;
		Ogre::Sphere bounds(mesh->getBounds().getCenter(), mesh->getBoundingSphereRadius());
		for (int i = 0; i < mesh->getNumSubMeshes(); i++) {
			m_meshNode->setSubMeshSphere(mesh->getSubMesh(i), bounds);
		}
	}

	Ogre::Matrix4 transform;
	makeTransform(
		transform,
		Ogre::Vector3(sphere.center.x, sphere.center.y, sphere.center.z),
		Ogre::Vector3(sphere.radius, sphere.radius, sphere.radius),
		Ogre::Quaternion()
	);
	addMeshInstance(mesh, transform, m_material);
}

void AdornRbxGfx::quad(
	const G3D::Vector3& v0,
	const G3D::Vector3& v1,
	const G3D::Vector3& v2,
	const G3D::Vector3& v3,
	const G3D::Color4& color
)
{
	float height = v0.y - v2.y;
	float depth = v0.z - v2.z;
	if (height < 0.0f) {
		height = -height;
	}
	if (depth < 0.0f) {
		depth = -depth;
	}
	float size = depth < height ? height : depth;

	G3D::Vector3 center(v0.x, (v0.y + v1.y + v2.y + v3.y) * 0.25, (v0.z + v1.z + v2.z + v3.z) * 0.25);
	Ogre::Sphere bounds(Ogre::Vector3(m_objectToWorld.translation + center), size * 1.4f);
	Ogre::Vector3 offset = reinterpret_cast<const Ogre::Vector3&>(m_objectToWorld.translation) -
						   m_visualEngine->m_camera->getDerivedPosition();
	if (!m_visualEngine->m_camera->isVisible(bounds, nullptr)) {
		return;
	}

	if (offset.squaredLength() > size * size * 60.0 * 60.0) {
		return;
	}

	setColor(color, 0);
	if (v0.x != v1.x || v1.x != v2.x || v2.x != v3.x) {
		throw std::runtime_error("Bad: quad not handled");
	}

	if (sphereMesh.isNull()) {
		Ogre::ManualObject* manual = m_visualEngine->getSceneManager()->createManualObject("ln" + nextMeshName());
		manual->begin("", Ogre::RenderOperation::OT_TRIANGLE_LIST);
		manual->position(0.0f, 0.5f, 0.5f);
		manual->normal(Ogre::Vector3(1.0f, 0.0f, 0.0f));
		manual->index(0);
		manual->position(0.0f, -0.5f, 0.5f);
		manual->normal(Ogre::Vector3(1.0f, 0.0f, 0.0f));
		manual->index(1);
		manual->position(0.0f, -0.5f, -0.5f);
		manual->normal(Ogre::Vector3(1.0f, 0.0f, 0.0f));
		manual->index(2);
		manual->position(0.0f, 0.5f, 0.5f);
		manual->normal(Ogre::Vector3(1.0f, 0.0f, 0.0f));
		manual->index(3);
		manual->position(0.0f, -0.5f, -0.5f);
		manual->normal(Ogre::Vector3(1.0f, 0.0f, 0.0f));
		manual->index(4);
		manual->position(0.0f, 0.5f, -0.5f);
		manual->normal(Ogre::Vector3(1.0f, 0.0f, 0.0f));
		manual->index(5);
		manual->end();
		sphereMesh = manual->convertToMesh(
			m_visualEngine->getSceneManager()->getName() + sphereMeshSuffix,
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
		);
		Ogre::Sphere meshBounds(sphereMesh->getBounds().getCenter(), sphereMesh->getBoundingSphereRadius());
		m_meshNode->setSubMeshSphere(sphereMesh->getSubMesh(0), meshBounds);
	}

	Ogre::Matrix4 transform;
	makeObjectTransform(transform);
	Ogre::Matrix4 local;
	local.makeTransform(
		reinterpret_cast<const Ogre::Vector3&>(center),
		Ogre::Vector3(1.0f, v2.y - v0.y, v2.z - v0.z),
		Ogre::Quaternion()
	);
	transform = transform * local;
	addMeshInstance(sphereMesh, transform, m_material);
}

void AdornRbxGfx::ray(const G3D::Ray& ray, const G3D::Color4& color, float scale)
{
	setColor(color, 0);
	Ogre::Quadric quadric;

	std::string key("ray0001");
	Ogre::MeshPtr mesh = findMesh(key);
	if (mesh.isNull()) {
		Ogre::Quadric quadric;
		Ogre::ManualObject* manual =
			quadric.cylinder(m_visualEngine->getSceneManager(), nextMeshName(), nullptr, 0.05f, 0.05f, 1.0f, 12, 1);
		quadric.setOffset(Ogre::Vector3(0.0f, 0.0f, 1.0f));
		quadric.cylinder(m_visualEngine->getSceneManager(), nextMeshName(), manual, 0.15f, 0.0f, 0.3f, 12, 1);
		quadric.orientation = 1;
		quadric.disk(m_visualEngine->getSceneManager(), nextMeshName(), manual, 0.0f, 0.15f, 12, 1);
		quadric.setOffset(Ogre::Vector3(0.0f, 0.0f, 0.0f));
		quadric.disk(m_visualEngine->getSceneManager(), nextMeshName(), manual, 0.0f, 0.05f, 12, 1);
		for (int i = 0; i < 4; i++) {
			manual->setMaterialName(i, m_material->getName());
		}
		mesh = manual->convertToMesh(
			m_visualEngine->m_sceneManager->getName() + "amsh" + Ogre::StringConverter::toString(m_meshes.size()),
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
		);
		m_meshes[key] = mesh;
		Ogre::Sphere bounds(mesh->getBounds().getCenter(), mesh->getBoundingSphereRadius());
		for (int i = 0; i < mesh->getNumSubMeshes(); i++) {
			m_meshNode->setSubMeshSphere(mesh->getSubMesh(i), bounds);
		}
	}

	float length = ray.direction.magnitude() / 1.3f;
	Ogre::Quaternion rotation =
		Ogre::Vector3(0.0f, 0.0f, 1.0f).getRotationTo(Ogre::Vector3(ray.direction.x, ray.direction.y, ray.direction.z));
	Ogre::Matrix4 transform;
	makeTransform(
		transform,
		Ogre::Vector3(ray.origin.x, ray.origin.y, ray.origin.z),
		Ogre::Vector3(length * 0.5, length * 0.5, length),
		rotation
	);
	addMeshInstance(mesh, transform, m_material);
}

AdornRbxGfx::~AdornRbxGfx()
{
	for (std::map<std::string, Ogre::MeshPtr>::const_iterator it = m_meshes.begin(); it != m_meshes.end(); ++it) {
		const Ogre::MeshPtr& mesh = it->second;
		Ogre::MeshManager::getSingleton().remove(mesh->getHandle());
	}
	m_meshes.clear();

	m_visualEngine->getSceneManager()->getRootSceneNode()->removeAndDestroyChild(m_rootNode->getName());
	m_visualEngine->getSceneManager()->getRootSceneNode()->removeChild(m_meshNode);
	delete m_meshNode;
	delete m_entity;

	for (int i = 0; i < (int) m_explosionNodes.size(); i++) {
		m_visualEngine->getSceneManager()->destroyParticleSystem(
			static_cast<Ogre::ParticleSystem*>(m_explosionNodes[i]->getAttachedObject((unsigned short) 0))->getName()
		);
		m_visualEngine->getSceneManager()->destroySceneNode(m_explosionNodes[i]->getName());
	}
	if (m_explosionRoot) {
		m_visualEngine->getSceneManager()->destroySceneNode(m_explosionRoot->getName());
	}
	m_explosionNodes.clear();
	m_activeExplosions.clear();
}

AdornRbxGfx::AdornRbxGfx(Ogre::VisualEngine* visualEngine) : m_textureName(""), m_explosionRoot(nullptr)
{
	m_visualEngine = visualEngine;
	if (adornFont.isNull()) {
		try {
			adornFont = Ogre::RbxFont::fromFile(ContentProvider::singleton().getAssetFile("Fonts\\comics.fnt"));
		}
		catch (...) {
		}
	}

	char name[8];
	snprintf(name, sizeof(name), "a3dr%d", adornCount++);
	m_rootNode = m_visualEngine->getSceneManager()->createSceneNode(name);
	m_visualEngine->getSceneManager()->getRootSceneNode()->addChild(m_rootNode);

	m_meshNode = new Ogre::RbxSceneNode(visualEngine->getSceneManager(), std::string(name) + "opt");
	m_visualEngine->getSceneManager()->getRootSceneNode()->addChild(m_meshNode);

	m_entity = new Ogre::RbxEntity();
	m_entity->m_frustumCull = false;
	m_entity->m_recordFrameStats = false;
	Ogre::VisualEngine* engine = m_visualEngine;
	m_entity->setFrameRateManager(&engine->getFrameRateManager());
	m_meshNode->attachObject(m_entity);

	Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().getByName("rbxdummymesh");
	if (mesh.isNull()) {
		mesh = Ogre::MeshManager::getSingleton().createOrRetrieve("rbxdummymesh", "General").first;
	}
	m_entity->setMesh(mesh);

	snprintf(name, sizeof(name), "loff%d", adornCount - 1);
	m_unlitMaterial = Ogre::MaterialManager::getSingleton().create(name, "General");
	m_unlitMaterial->setLightingEnabled(false);
}

void AdornRbxGfx::explosion(const G3D::Sphere& sphere, const G3D::Color4& solid, const G3D::Color4& wire)
{
	Ogre::Vector3 position;
	{
		Ogre::Quaternion orientation;
		const Ogre::Matrix3& rotation = reinterpret_cast<const Ogre::Matrix3&>(m_objectToWorld.rotation);
		orientation.FromRotationMatrix(rotation);
		position =
			m_rootNode
				->createChildSceneNode(reinterpret_cast<const Ogre::Vector3&>(m_objectToWorld.translation), orientation)
				->getPosition();
	}

	char key[64];
	snprintf(key, sizeof(key), "%.3f %.3f %.3f", position.x, position.y, position.z);
	Ogre::LogManager::getSingleton().logMessage(
		std::string("RBX expl ") + " r=" + Ogre::StringConverter::toString(sphere.radius)
	);

	if (m_activeExplosions.contains(key)) {
		Ogre::ParticleSystem* particleSystem =
			static_cast<Ogre::ParticleSystem*>(m_activeExplosions[key]->getAttachedObject((unsigned short) 0));
		if (particleSystem->getNumParticles() == 0) {
			Ogre::ParticleEmitter* emitter = particleSystem->getEmitter(0);
			particleSystem->setParticleQuota((size_t) (atoi("400") * 1.0f));
			emitter->setMinParticleVelocity(pow(1.0f, 1.0f / 3.0f) * atoi("20"));
			emitter->setMaxParticleVelocity(pow(1.0f, 1.0f / 3.0f) * atoi("30"));
			emitter->setMinDuration((1.0f - 1.0) * 0.4f + atoi("1.2"));
			emitter->setMaxDuration((1.0f - 1.0) * 0.4f + atoi("1.2"));
			emitter->setEnabled(false);
			m_activeExplosions.erase(m_activeExplosions.find(key));
		}
		return;
	}

	std::vector<std::map<std::string, Ogre::SceneNode*>::iterator> retired;
	std::map<std::string, Ogre::SceneNode*>::iterator it;
	for (it = m_activeExplosions.begin(); it != m_activeExplosions.end(); ++it) {
		Ogre::ParticleSystem* particleSystem =
			static_cast<Ogre::ParticleSystem*>(it->second->getAttachedObject((unsigned short) 0));
		if (particleSystem->getNumParticles() == 0) {
			retired.push_back(it);
		}
	}

	for (unsigned int i = 0; i < retired.size(); i++) {
		Ogre::ParticleSystem* particleSystem =
			static_cast<Ogre::ParticleSystem*>(retired[i]->second->getAttachedObject((unsigned short) 0));
		Ogre::ParticleEmitter* emitter = particleSystem->getEmitter(0);
		emitter->setEnabled(false);
		m_activeExplosions.erase(retired[i]);
		Ogre::LogManager::getSingleton().logMessage("RBX retired " + particleSystem->getName());
		particleSystem->setParticleQuota((size_t) (atoi("400") * 1.0f));
		emitter->setMinParticleVelocity(pow(1.0f, 1.0f / 3.0f) * atoi("20"));
		emitter->setMaxParticleVelocity(pow(1.0f, 1.0f / 3.0f) * atoi("30"));
		emitter->setMinDuration((1.0f - 1.0) * 0.4f + atoi("1.2"));
		emitter->setMaxDuration((1.0f - 1.0) * 0.4f + atoi("1.2"));
	}

	float scale = sphere.radius * 0.25;
	if (scale > 3.0f) {
		scale = 3.0f;
	}

	Ogre::SceneNode* node;
	unsigned int i;
	for (i = 0; i < m_explosionNodes.size(); i++) {
		Ogre::ParticleSystem* particleSystem =
			static_cast<Ogre::ParticleSystem*>(m_explosionNodes[i]->getAttachedObject((unsigned short) 0));
		if (!particleSystem->getEmitter(0)->getEnabled()) {
			node = m_explosionNodes[i];
			m_activeExplosions[key] = node;
			node->setPosition(position.x, position.y, position.z);
			particleSystem = static_cast<Ogre::ParticleSystem*>(node->getAttachedObject((unsigned short) 0));
			Ogre::ParticleEmitter* emitter = particleSystem->getEmitter(0);
			particleSystem->setParticleQuota((size_t) (atoi("400") * scale));
			emitter->setMinParticleVelocity(pow(scale, 1.0f / 3.0f) * atoi("20"));
			emitter->setMaxParticleVelocity(pow(scale, 1.0f / 3.0f) * atoi("30"));
			emitter->setMinDuration((scale - 1.0) * 0.4f + atoi("1.2"));
			emitter->setMaxDuration((scale - 1.0) * 0.4f + atoi("1.2"));
			emitter->setEnabled(true);
			particleSystem->_update(0.001f);
			Ogre::LogManager::getSingleton().logMessage("RBX use " + particleSystem->getName());
			break;
		}
	}

	if (i == m_explosionNodes.size()) {
		if (!Ogre::MaterialManager::getSingleton().resourceExists("explosion/explosionMatl")) {
			Ogre::DataStreamPtr materialStream(new Ogre::MemoryDataStream(
				const_cast<char*>(explosionMaterialScript.c_str()), explosionMaterialScript.size() + 1, false
			));
			Ogre::MaterialManager::getSingleton().parseScript(materialStream, "General");
			Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName("explosion/explosionMatl");
			material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(
				ContentProvider::singleton().getAssetFile("textures\\explosion.png"), Ogre::TEX_TYPE_2D
			);
			Ogre::DataStreamPtr particleStream(new Ogre::MemoryDataStream(
				const_cast<char*>(explosionParticleScript.c_str()), explosionParticleScript.size() + 1, false
			));
			Ogre::ParticleSystemManager::getSingleton().parseScript(particleStream, "General");
		}

		if (!m_explosionRoot) {
			m_explosionRoot = m_visualEngine->getSceneManager()->getRootSceneNode()->createChildSceneNode(
				m_visualEngine->getSceneManager()->getName() + "rbxexpl",
				Ogre::Vector3::ZERO,
				Ogre::Quaternion::IDENTITY
			);
		}

		Ogre::ParticleSystem* particleSystem = m_visualEngine->getSceneManager()->createParticleSystem(
			m_visualEngine->getSceneManager()->getName() + "expl" + Ogre::StringConverter::toString(i),
			"explosion/explosion1"
		);
		if (!particleSystem) {
			Ogre::LogManager::getSingleton().logMessage("RBX Error creating psys");
			return;
		}

		Ogre::ParticleEmitter* emitter = particleSystem->getEmitter(0);
		particleSystem->setParticleQuota((size_t) (atoi("400") * scale));
		emitter->setMinParticleVelocity(pow(scale, 1.0f / 3.0f) * atoi("20"));
		emitter->setMaxParticleVelocity(pow(scale, 1.0f / 3.0f) * atoi("30"));
		emitter->setMinDuration((scale - 1.0) * 0.4f + atoi("1.2"));
		emitter->setMaxDuration((scale - 1.0) * 0.4f + atoi("1.2"));
		node = m_explosionRoot->createChildSceneNode(Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY);
		node->attachObject(particleSystem);
		m_explosionNodes.push_back(node);
		m_activeExplosions[key] = node;
		Ogre::LogManager::getSingleton().logMessage("RBX created " + particleSystem->getName());
	}

	node->setPosition(position.x, position.y, position.z);
}

void AdornRbxGfx::lineSegment(const G3D::LineSegment& segment, const G3D::Color4& color, float scale)
{
}

void AdornRbxGfx::axes(const G3D::Color4& x, const G3D::Color4& y, const G3D::Color4& z, float scale)
{
}

} // namespace RBX
