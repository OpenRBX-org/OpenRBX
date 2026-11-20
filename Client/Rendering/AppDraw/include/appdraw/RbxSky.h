#ifndef APPDRAW_RBXSKY_H
#define APPDRAW_RBXSKY_H

#include <OgreMaterial.h>
#include <OgreString.h>
#include <OgreTexture.h>
#include <OgreVector3.h>
#include <vector>

namespace G3D {
class LightingParameters;
} // namespace G3D

namespace Ogre {

class BillboardSet;
class Camera;
class RenderQueue;
class SceneManager;
class SceneNode;
class VisualEngine;

class RbxSky
{
public:
	RbxSky(SceneManager* sceneManager, const String& folder, bool celestialBodies);
	virtual ~RbxSky();

	void updateMoon(VisualEngine* engine, const G3D::LightingParameters& parameters);
	void updateSun(VisualEngine* engine, const G3D::LightingParameters& parameters);
	void update(VisualEngine* engine, const G3D::LightingParameters& parameters);
	void updateRenderQueue(Camera* cam, RenderQueue* queue, uint8 queueGroupId);

private:
	TexturePtr m_sunTexture;
	TexturePtr m_moonTexture;
	TexturePtr m_unk0x1c;
	TexturePtr m_sunRaysTexture;
	BillboardSet* m_sun;
	BillboardSet* m_moon;
	BillboardSet* m_sunRays;
	SceneNode* m_sunNode;
	SceneNode* m_moonNode;
	SceneNode* m_starsNode;
	SceneNode* m_skyNode;
	MaterialPtr m_sunMaterial;
	MaterialPtr m_sunRaysMaterial;
	MaterialPtr m_moonMaterial;
	MaterialPtr m_starsMaterial;
	bool m_sunRaysVisible;
	bool m_starsVisible;
	std::vector<Vector3> m_starPositions;
	std::vector<Real> m_starBrightness;
	bool m_celestialBodies;
	Vector3 m_sunPosition;
	Vector3 m_moonPosition;
};

} // namespace Ogre

#endif // APPDRAW_RBXSKY_H
