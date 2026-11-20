#include "appdraw/RbxSky.h"

#include "appdraw/VisualEngine.h"

#include <G3D/BinaryInput.h>
#include <G3D/fileutils.h>
#include <G3D/g3dmath.h>
#include <GLG3D/LightingParameters.h>
#include <OgreBillboard.h>
#include <OgreBillboardSet.h>
#include <OgreCamera.h>
#include <OgreManualObject.h>
#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgrePass.h>
#include <OgreRenderQueue.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreStringConverter.h>
#include <OgreSubMesh.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <OgreTextureUnitState.h>
#include <OgreViewport.h>
#include <algorithm>

namespace Ogre {

void RbxSky::updateMoon(VisualEngine* engine, const G3D::LightingParameters& parameters)
{
	if (parameters.moonPosition.y > -0.3f) {
		m_starsVisible = true;

		MeshPtr mesh = MeshManager::getSingleton().getByName("starsMesh");
		if (!mesh.isNull()) {
			if (0.3f < parameters.skyAmbient.r && parameters.skyAmbient.r < 0.7f) {
				for (int i = 0; i < 3; i++) {
					MaterialPtr material =
						MaterialManager::getSingleton().getByName(mesh->getSubMesh(i)->getMaterialName());
					material->setSceneBlending(SBF_ONE_MINUS_DEST_COLOUR, SBF_ZERO);
				}
			}
			else {
				for (int i = 0; i < 3; i++) {
					MaterialPtr material =
						MaterialManager::getSingleton().getByName(mesh->getSubMesh(i)->getMaterialName());
					material->setSceneBlending(SBT_REPLACE);
				}
			}
		}
	}
	else {
		m_starsVisible = false;
	}

	G3D::Vector3 position = parameters.physicallyCorrect ? parameters.trueMoonPosition : parameters.moonPosition;
	float brightness = std::min(1.0f - parameters.skyAmbient.g + 0.1f, 1.0f);
	position *= 2700.0;
	m_moonNode->setPosition(position.x, position.y, position.z);
	m_moon->getBillboard(0)->setColour(ColourValue(brightness, brightness, brightness, 1.0f));
	m_moonPosition = Vector3(position.x, position.y, position.z);
}

void RbxSky::updateSun(VisualEngine* engine, const G3D::LightingParameters& parameters)
{
	G3D::Vector3 position = parameters.physicallyCorrect ? parameters.trueSunPosition : parameters.sunPosition;
	G3D::Color3 colour = parameters.emissiveScale * 0.8;

	if (position.y < 0.0f) {
		float fade = std::max(0.0f, (float) ((0.1f + position.y) * 10.0));
		colour *= fade;
	}

	position *= 2700.0;
	m_sunNode->setPosition(position.x, position.y, position.z);
	m_sun->getBillboard(0)->setColour(ColourValue(colour.r, colour.g, colour.b, 1.0f));
	m_sun->getBillboard(1)->setColour(ColourValue(colour.r, colour.g, colour.b, 1.0f));
	m_sun->getBillboard(2)->setColour(ColourValue(colour.r, colour.g, colour.b, 1.0f));
	m_sunPosition = Vector3(position.x, position.y, position.z);

	if (position.x > 0.0f && position.y >= -0.1) {
		float rays = std::max(0.0f, std::min((float) (1.0f - position.y * 2.0 / sqrt(2.0)), 1.0f));
		float alpha = rays * 0.256f;
		m_sunRays->getBillboard(0)->setColour(ColourValue(alpha, alpha, alpha, alpha));
		m_sunRaysVisible = true;
	}
	else {
		m_sunRaysVisible = false;
	}
}

void RbxSky::update(VisualEngine* engine, const G3D::LightingParameters& parameters)
{
	engine->getViewport()->setBackgroundColour(
		ColourValue(parameters.skyAmbient.r, parameters.skyAmbient.g, parameters.skyAmbient.b, 1.0f)
	);
	if (m_celestialBodies) {
		updateMoon(engine, parameters);
		updateSun(engine, parameters);
	}
}

void RbxSky::updateRenderQueue(Camera* cam, RenderQueue* queue, uint8 queueGroupId)
{
	Vector3 position = cam->getDerivedPosition();
	m_moonNode->setPosition(m_moonPosition + position);
	m_sunNode->setPosition(m_sunPosition + position);
	m_starsNode->setPosition(position);

	queue->getQueueGroup(RENDER_QUEUE_1)->setShadowsEnabled(false);
	queue->getQueueGroup(RENDER_QUEUE_2)->setShadowsEnabled(false);

	m_moon->setRenderQueueGroup(RENDER_QUEUE_1);
	m_moon->_notifyCurrentCamera(cam);
	m_moon->_updateRenderQueue(queue);

	m_sun->setRenderQueueGroup(RENDER_QUEUE_2);
	m_sun->_notifyCurrentCamera(cam);
	m_sun->_updateRenderQueue(queue);

	if (m_sunRaysVisible) {
		m_sunRays->setRenderQueueGroup(RENDER_QUEUE_2);
		m_sunRays->_notifyCurrentCamera(cam);
		m_sunRays->_updateRenderQueue(queue);
	}

	if (m_starsVisible) {
		SceneNode::ObjectIterator it = m_starsNode->getAttachedObjectIterator();
		while (it.hasMoreElements()) {
			ManualObject* stars = dynamic_cast<ManualObject*>(it.getNext());
			stars->setRenderQueueGroup(RENDER_QUEUE_1);
			stars->_notifyCurrentCamera(cam);
			stars->_updateRenderQueue(queue);
		}
	}
}

RbxSky::~RbxSky() = default;

RbxSky::RbxSky(SceneManager* sceneManager, const String& folder, bool celestialBodies) : m_sunRaysVisible(false)
{
	m_celestialBodies = celestialBodies;
	if (!m_celestialBodies) {
		return;
	}

	const String& name = sceneManager->getName();

	m_moonTexture = TextureManager::getSingleton().load(folder + "moon.jpg", "General");
	m_sunTexture = TextureManager::getSingleton().load(folder + "sun.jpg", "General");
	m_sunRaysTexture = TextureManager::getSingleton().load(folder + "sun-rays.jpg", "General");

	m_sunMaterial = MaterialManager::getSingleton().create(name + "sunMatl", "General");
	m_sunMaterial->setSceneBlending(SBF_ONE, SBF_ONE);
	TextureUnitState* sunUnit = m_sunMaterial->getTechnique(0)->getPass(0)->createTextureUnitState();
	sunUnit->setColourOperationEx(LBX_MODULATE, LBS_TEXTURE, LBS_CURRENT);
	sunUnit->setAlphaOperation(LBX_MODULATE, LBS_TEXTURE, LBS_CURRENT);
	sunUnit->setTextureName(m_sunTexture->getName());
	m_sunMaterial->setDepthWriteEnabled(false);
	m_sunMaterial->setLightingEnabled(false);

	m_sunRaysMaterial = m_sunMaterial->clone(name + "sunRaysMatl");
	m_sunRaysMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(m_sunRaysTexture->getName());

	m_moonMaterial = MaterialManager::getSingleton().create(name + "moonMatl", "General");
	m_moonMaterial->setSceneBlending(SBF_SOURCE_ALPHA, SBF_ONE);
	TextureUnitState* moonUnit = m_moonMaterial->getTechnique(0)->getPass(0)->createTextureUnitState();
	moonUnit->setColourOperationEx(LBX_MODULATE, LBS_TEXTURE, LBS_CURRENT);
	moonUnit->setAlphaOperation(LBX_MODULATE, LBS_TEXTURE, LBS_CURRENT);
	moonUnit->setTextureName(m_moonTexture->getName());
	moonUnit->setTextureName(m_moonTexture->getName());
	m_moonMaterial->setDepthWriteEnabled(false);
	m_moonMaterial->setLightingEnabled(false);

	m_sun = sceneManager->createBillboardSet(name + "sun", 1);
	m_sun->setDefaultDimensions(1000.0f, 1000.0f);
	m_sun->createBillboard(Vector3::ZERO, ColourValue::White);
	m_sun->createBillboard(Vector3::ZERO, ColourValue::White);
	m_sun->createBillboard(Vector3::ZERO, ColourValue::White);
	m_sun->setMaterialName(name + "sunMatl");

	m_sunRays = sceneManager->createBillboardSet(name + "sunRays", 1);
	m_sunRays->setDefaultDimensions(1000.0f, 1000.0f);
	m_sunRays->createBillboard(Vector3::ZERO, ColourValue::White);
	m_sunRays->setMaterialName(name + "sunRaysMatl");

	m_moon = sceneManager->createBillboardSet(name + "moon", 1);
	m_moon->setDefaultDimensions(500.0f, 500.0f);
	m_moon->createBillboard(Vector3::ZERO, ColourValue::White);
	m_moon->setMaterialName(name + "moonMatl");

	m_skyNode = sceneManager->createSceneNode(name + "skyNode");
	m_sunNode = m_skyNode->createChildSceneNode(name + "sunNode");
	m_moonNode = m_skyNode->createChildSceneNode(name + "moonNode");
	m_sunNode->attachObject(m_sun);
	m_sunNode->attachObject(m_sunRays);
	m_moonNode->attachObject(m_moon);
	m_sun->setCastShadows(false);
	m_sunRays->setCastShadows(false);
	m_moon->setCastShadows(false);

	if (G3D::fileExists(folder + "real.str")) {
		G3D::BinaryInput in(folder + "real.str", G3D::G3D_LITTLE_ENDIAN, true);
		in.readString(5);

		int count = in.readInt16();
		m_starPositions.resize(count);
		m_starBrightness.resize(count, 0.0f);

		for (int i = 0; i < count; i++) {
			float x = (in.readInt16() * 2.0 + 1.0) * (1.0 / 65535.0);
			float y = (in.readInt16() * 2.0 + 1.0) * (1.0 / 65535.0);
			float z = (in.readInt16() * 2.0 + 1.0) * (1.0 / 65535.0);
			m_starPositions[i] = Vector3(x * 1000.0, y * 1000.0, z * 1000.0);
			m_starBrightness[i] = G3D::square((in.readInt16() * 2.0 + 1.0) * (1.0 / 65535.0)) + 0.3f;
		}
	}
	else {
		m_starPositions.resize(3000);
		m_starBrightness.resize(m_starPositions.size(), 0.0f);

		for (int i = m_starPositions.size() - 1; i >= 0; --i) {
			G3D::Vector3 direction = G3D::Vector3::random();
			m_starPositions[i] = Vector3(direction.x * 1000.0, direction.y * 1000.0, direction.z * 1000.0);
			m_starBrightness[i] = G3D::square(G3D::uniformRandom()) + 0.3f;
		}
	}

	m_starsNode = sceneManager->createSceneNode(name + "starsNode");

	for (int i = 1; i <= 3; i++) {
		m_starsMaterial =
			MaterialManager::getSingleton().create(name + "starsMatl" + StringConverter::toString(i), "General");
		m_starsMaterial->setDepthWriteEnabled(false);
		m_starsMaterial->setLightingEnabled(false);

		Real brightness = i;
		m_starsMaterial->setPointSize(brightness + 0.2f);

		String starsName = name + "st" + StringConverter::toString(i);
		ManualObject* stars = new ManualObject(starsName);
		stars->begin(m_starsMaterial->getName(), RenderOperation::OT_POINT_LIST);
		for (int j = 0; j < (int) m_starPositions.size(); j++) {
			if (i - 1 < m_starBrightness[j] * 3.0 && m_starBrightness[j] * 3.0 <= brightness) {
				stars->position(m_starPositions[j].x, m_starPositions[j].y, m_starPositions[j].z);
			}
		}
		stars->end();
		stars->setMaterialName(0, m_starsMaterial->getName());
		m_starsNode->attachObject(stars);
		stars->setCastShadows(false);
	}
}

} // namespace Ogre
