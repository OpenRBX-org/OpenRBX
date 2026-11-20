#ifndef APPDRAW_RBXSCENEMANAGER_H
#define APPDRAW_RBXSCENEMANAGER_H

#include "appdraw/RbxSky.h"

#include <OgreSceneManager.h>
#include <OgreSceneManagerEnumerator.h>

namespace RBX::ViewNew {
class View;
} // namespace RBX::ViewNew

namespace Ogre {

class RbxSceneManager : public SceneManager
{
public:
	class SceneMgrQueuedRenderableVisitor : public QueuedRenderableVisitor
	{
	protected:
		const Pass* mUsedPass;

	public:
		SceneMgrQueuedRenderableVisitor() : transparentShadowCastersMode(false) {}
		~SceneMgrQueuedRenderableVisitor() override = default;

		void visit(const Renderable* r) override;
		bool visit(const Pass* p) override;
		void visit(const RenderablePass* rp) override;

		RbxSceneManager* targetSceneMgr;
		bool transparentShadowCastersMode;
		bool autoLights;
		const LightList* manualLightList;
	};

	class ShadowCasterSceneQueryListener : public SceneQueryListener
	{
	protected:
		SceneManager* mSceneMgr;
		ShadowCasterList* mCasterList;
		bool mIsLightInFrustum;
		const PlaneBoundedVolumeList* mLightClipVolumeList;
		const Camera* mCamera;
		const Light* mLight;
		Real mFarDistSquared;

	public:
		ShadowCasterSceneQueryListener(SceneManager* sm)
			: mSceneMgr(sm), mCasterList(nullptr), mIsLightInFrustum(false), mLightClipVolumeList(nullptr),
			  mCamera(nullptr)
		{
		}

		void prepare(
			bool lightInFrustum,
			const PlaneBoundedVolumeList* lightClipVolumes,
			const Light* light,
			const Camera* cam,
			ShadowCasterList* casterList,
			Real farDistSquared
		)
		{
			mCasterList = casterList;
			mIsLightInFrustum = lightInFrustum;
			mLightClipVolumeList = lightClipVolumes;
			mCamera = cam;
			mLight = light;
			mFarDistSquared = farDistSquared;
		}

		bool queryResult(MovableObject* object) override;
		bool queryResult(SceneQuery::WorldFragment* fragment) override { return true; }
	};

protected:
	struct lightsForShadowTextureLess
	{
		bool operator()(const Light* l1, const Light* l2) const;
	};

	bool m_lightingEnabled;
	bool m_unk0x8a65;
	String m_skyFolder;
	SharedPtr<RbxSky> m_sky;
	SceneNode* m_skyBoxNode;
	ShadowCasterSceneQueryListener* mShadowCasterQueryListener;
	SceneMgrQueuedRenderableVisitor* mActiveQueuedRenderableVisitor;
	SceneMgrQueuedRenderableVisitor mDefaultQueuedRenderableVisitor;

	friend class SceneMgrQueuedRenderableVisitor;
	friend class RBX::ViewNew::View;

	MeshPtr createSkyboxPlane(BoxPlane bp, Real distance, const Quaternion& orientation, const String& groupName);
	MeshPtr createSkydomePlane(
		BoxPlane bp,
		Real curvature,
		Real tiling,
		Real distance,
		const Quaternion& orientation,
		int xsegments,
		int ysegments,
		int ySegmentsToKeep,
		const String& groupName
	);
	bool fireRenderQueueStarted(uint8 id, const String& invocation);
	bool fireRenderQueueEnded(uint8 id, const String& invocation);
	void fireShadowTexturesUpdated(size_t numberOfShadowTextures);
	void fireShadowTexturesPreCaster(Light* light, Camera* camera);
	void fireShadowTexturesPreReceiver(Light* light, Frustum* f);
	void useRenderableViewProjMode(const Renderable* pRend);
	void resetViewProjMode();
	const Pass* deriveShadowCasterPass(const Pass* pass);
	const Pass* deriveShadowReceiverPass(const Pass* pass);
	void renderShadowVolumeObjects(
		ShadowCaster::ShadowRenderableListIterator iShadowRenderables,
		Pass* pass,
		const LightList* manualLightList,
		unsigned long flags,
		bool secondpass,
		bool zfail,
		bool twosided
	);
	const MovableObjectCollection* getMovableObjectCollection(const String& typeName) const;
	MovableObjectCollection* getMovableObjectCollection(const String& typeName);

	bool validatePassForRendering(const Pass* pass);
	bool validateRenderableForRendering(const Pass* pass, const Renderable* rend);

public:
	enum PrefabType
	{
		PT_PLANE,
		PT_CUBE,
		PT_SPHERE
	};

	using SceneManager::createEntity;

	const ShadowCasterList& getShadowCasterList() const { return mShadowCasterList; }

	RbxSceneManager(const String& name);
	~RbxSceneManager() override;

	const String& getTypeName() const override;

	void clearScene() override;
	void destroyAllCameras() override;

	void setViewport(Viewport* vp) override;
	void renderObjects(
		const QueuedRenderableCollection& objs,
		QueuedRenderableCollection::OrganisationMode om,
		bool doLightIteration,
		const LightList* manualLightList = nullptr
	) override;
	bool hasCamera(const String& name) const override;
	Light* createLight(const String& name) override;
	Light* getLight(const String& name) const override;
	bool hasLight(const String& name) const override;
	void destroyLight(const String& name) override;
	void destroyLight(Light* light) override;
	void destroyAllLights() override;
	void _notifyLightsDirty() override;
	const LightList& _getLightsAffectingFrustum() const override;
	virtual Entity* createEntity(const String& entityName, PrefabType ptype);
	SceneNode* getRootSceneNode() const override;
	bool hasSceneNode(const String& name) const override;
	Entity* getEntity(const String& name) const override;
	bool hasEntity(const String& name) const override;
	void destroyEntity(Entity* ent) override;
	void destroyEntity(const String& name) override;
	void destroyAllEntities() override;
	ManualObject* createManualObject(const String& name) override;
	ManualObject* getManualObject(const String& name) const override;
	bool hasManualObject(const String& name) const override;
	void destroyManualObject(ManualObject* obj) override;
	void destroyManualObject(const String& name) override;
	void destroyAllManualObjects() override;
	BillboardChain* createBillboardChain(const String& name) override;
	BillboardChain* getBillboardChain(const String& name) const override;
	bool hasBillboardChain(const String& name) const override;
	void destroyBillboardChain(BillboardChain* obj) override;
	void destroyBillboardChain(const String& name) override;
	void destroyAllBillboardChains() override;
	RibbonTrail* createRibbonTrail(const String& name) override;
	RibbonTrail* getRibbonTrail(const String& name) const override;
	bool hasRibbonTrail(const String& name) const override;
	void destroyRibbonTrail(RibbonTrail* obj) override;
	void destroyRibbonTrail(const String& name) override;
	void destroyAllRibbonTrails() override;
	ParticleSystem* getParticleSystem(const String& name) const override;
	bool hasParticleSystem(const String& name) const override;
	void destroyParticleSystem(ParticleSystem* obj) override;
	void destroyParticleSystem(const String& name) override;
	void destroyAllParticleSystems() override;
	void _updateSceneGraph(Camera* cam) override;
	void _findVisibleObjects(Camera* cam, VisibleObjectsBoundsInfo* visibleBounds, bool onlyShadowCasters) override;
	void _renderVisibleObjects() override;
	bool isSkyPlaneEnabled() const override { return mSkyPlaneEnabled; }
	SceneNode* getSkyPlaneNode() const override { return mSkyPlaneNode; }
	const SkyPlaneGenParameters& getSkyPlaneGenParameters() const override { return mSkyPlaneGenParameters; }
	bool isSkyBoxEnabled() const override { return mSkyBoxEnabled; }
	SceneNode* getSkyBoxNode() const override { return mSkyBoxNode; }
	const SkyBoxGenParameters& getSkyBoxGenParameters() const override { return mSkyBoxGenParameters; }
	bool isSkyDomeEnabled() const override { return mSkyDomeEnabled; }
	SceneNode* getSkyDomeNode() const override { return mSkyDomeNode; }
	const SkyDomeGenParameters& getSkyDomeGenParameters() const override { return mSkyDomeGenParameters; }
	const ColourValue& getFogColour() const override;
	Real getFogStart() const override;
	Real getFogEnd() const override;
	Real getFogDensity() const override;
	BillboardSet* getBillboardSet(const String& name) const override;
	bool hasBillboardSet(const String& name) const override;
	void destroyBillboardSet(BillboardSet* set) override;
	void destroyBillboardSet(const String& name) override;
	void destroyAllBillboardSets() override;
	void setDisplaySceneNodes(bool display) override;
	bool getDisplaySceneNodes() const override { return mDisplayNodes; }
	AnimationState* getAnimationState(const String& animName) const override;
	bool hasAnimationState(const String& name) const override;
	void destroyAnimationState(const String& name) override;
	void destroyAllAnimationStates() override;
	RenderQueue* getRenderQueue() override;
	void addRenderQueueListener(RenderQueueListener* newListener) override;
	void addSpecialCaseRenderQueue(uint8 qid) override;
	void removeSpecialCaseRenderQueue(uint8 qid) override;
	void clearSpecialCaseRenderQueues() override;
	void setSpecialCaseRenderQueueMode(SpecialCaseRenderQueueMode mode) override;
	SpecialCaseRenderQueueMode getSpecialCaseRenderQueueMode() override;
	void setWorldGeometryRenderQueue(uint8 qid) override;
	uint8 getWorldGeometryRenderQueue() override;
	void showBoundingBoxes(bool bShow) override;
	bool getShowBoundingBoxes() const override;
	void _notifyAutotrackingSceneNode(SceneNode* node, bool autoTrack) override;
	AxisAlignedBoxSceneQuery* createAABBQuery(const AxisAlignedBox& box, unsigned long mask = 0xFFFFFFFF) override;
	SphereSceneQuery* createSphereQuery(const Sphere& sphere, unsigned long mask = 0xFFFFFFFF) override;
	PlaneBoundedVolumeListSceneQuery* createPlaneBoundedVolumeQuery(
		const PlaneBoundedVolumeList& volumes, unsigned long mask = 0xFFFFFFFF
	) override;
	RaySceneQuery* createRayQuery(const Ray& ray, unsigned long mask = 0xFFFFFFFF) override;
	IntersectionSceneQuery* createIntersectionQuery(unsigned long mask = 0xFFFFFFFF) override;
	void destroyQuery(SceneQuery* query) override;
	ShadowTechnique getShadowTechnique() const override { return mShadowTechnique; }
	void setShowDebugShadows(bool debug) override { mDebugShadows = debug; }
	bool getShowDebugShadows() const override { return mDebugShadows; }
	const ColourValue& getShadowColour() const override;
	void setShadowDirectionalLightExtrusionDistance(Real dist) override;
	Real getShadowDirectionalLightExtrusionDistance() const override;
	void setShadowFarDistance(Real distance) override;
	Real getShadowFarDistance() const override { return mShadowFarDist; }
	size_t getShadowIndexBufferSize() const override { return mShadowIndexBufferSize; }
	void setShadowTextureConfig(
		size_t shadowIndex, unsigned short width, unsigned short height, PixelFormat format
	) override;
	void setShadowDirLightTextureOffset(Real offset) override { mShadowTextureOffset = offset; }
	Real getShadowDirLightTextureOffset() const override { return mShadowTextureOffset; }
	void setShadowTextureFadeStart(Real fadeStart) override { mShadowTextureFadeStart = fadeStart; }
	void setShadowTextureFadeEnd(Real fadeEnd) override { mShadowTextureFadeEnd = fadeEnd; }
	void setShadowTextureSelfShadow(bool selfShadow) override;
	bool getShadowTextureSelfShadow() const override { return mShadowTextureSelfShadow; }
	void setShadowCasterRenderBackFaces(bool bf) override { mShadowCasterRenderBackFaces = bf; }
	bool getShadowCasterRenderBackFaces() const override { return mShadowCasterRenderBackFaces; }
	const ShadowCameraSetupPtr& getShadowCameraSetup() const override;
	void setShadowUseInfiniteFarPlane(bool enable) override { mShadowUseInfiniteFarPlane = enable; }
	bool isShadowTechniqueStencilBased() const override { return (mShadowTechnique & SHADOWDETAILTYPE_STENCIL) != 0; }
	bool isShadowTechniqueTextureBased() const override { return (mShadowTechnique & SHADOWDETAILTYPE_TEXTURE) != 0; }
	bool isShadowTechniqueModulative() const override { return (mShadowTechnique & SHADOWDETAILTYPE_MODULATIVE) != 0; }
	bool isShadowTechniqueAdditive() const override { return (mShadowTechnique & SHADOWDETAILTYPE_ADDITIVE) != 0; }
	bool isShadowTechniqueIntegrated() const override { return (mShadowTechnique & SHADOWDETAILTYPE_INTEGRATED) != 0; }
	bool isShadowTechniqueInUse() const override { return mShadowTechnique != SHADOWTYPE_NONE; }
	void addShadowListener(ShadowListener* s) override;
	bool hasStaticGeometry(const String& name) const override;
	void destroyStaticGeometry(StaticGeometry* geom) override;
	void destroyInstancedGeometry(InstancedGeometry* geom) override;
	void destroyMovableObject(MovableObject* m) override;
	void injectMovableObject(MovableObject* m) override;
	void extractMovableObject(MovableObject* m) override;
	void extractAllMovableObjectsByType(const String& typeName) override;
	void setVisibilityMask(uint32 vmask) override { mVisibilityMask = vmask; }
	uint32 getVisibilityMask() override { return mVisibilityMask; }
	void setFindVisibleObjects(bool find) override { mFindVisibleObjects = find; }
	bool getFindVisibleObjects() override { return mFindVisibleObjects; }
	void _injectRenderWithPass(Pass* pass, Renderable* rend, bool shadowDerivation = true) override;
	void _suppressRenderStateChanges(bool suppress) override;
	bool _areRenderStateChangesSuppressed() const override { return mSuppressRenderStateChanges; }
	void _suppressShadows(bool suppress) override;
	bool _areShadowsSuppressed() const override { return mSuppressShadows; }

	void initRenderQueue() override;
	void renderVisibleObjectsDefaultSequence() override;
	void renderVisibleObjectsCustomSequence(RenderQueueInvocationSequence* s) override;
	void prepareRenderQueue() override;
	void renderSingleObject(
		const Renderable* rend, const Pass* pass, bool doLightIteration, const LightList* manualLightList = nullptr
	) override;
	void findLightsAffectingFrustum(const Camera* camera) override;
	void initShadowVolumeMaterials() override;
	void ensureShadowTexturesCreated() override;
	void destroyShadowTextures() override;
	void prepareShadowTextures(Camera* cam, Viewport* vp) override;
	void renderShadowVolumesToStencil(const Light* light, const Camera* cam) override;
	void setShadowVolumeStencilState(bool secondpass, bool zfail, bool twosided) override;
	const ShadowCasterList& findShadowCastersForLight(const Light* light, const Camera* camera) override;
	void renderBasicQueueGroupObjects(
		RenderQueueGroup* pGroup, QueuedRenderableCollection::OrganisationMode om
	) override;
	void renderAdditiveStencilShadowedQueueGroupObjects(
		RenderQueueGroup* group, QueuedRenderableCollection::OrganisationMode om
	) override;
	void renderModulativeStencilShadowedQueueGroupObjects(
		RenderQueueGroup* group, QueuedRenderableCollection::OrganisationMode om
	) override;
	void renderTextureShadowCasterQueueGroupObjects(
		RenderQueueGroup* group, QueuedRenderableCollection::OrganisationMode om
	) override;
	void renderTextureShadowReceiverQueueGroupObjects(
		RenderQueueGroup* group, QueuedRenderableCollection::OrganisationMode om
	) override;
	void renderModulativeTextureShadowedQueueGroupObjects(
		RenderQueueGroup* group, QueuedRenderableCollection::OrganisationMode om
	) override;
	void renderAdditiveTextureShadowedQueueGroupObjects(
		RenderQueueGroup* group, QueuedRenderableCollection::OrganisationMode om
	) override;
	void renderTransparentShadowCasterObjects(
		const QueuedRenderableCollection& objs,
		QueuedRenderableCollection::OrganisationMode om,
		bool doLightIteration,
		const LightList* manualLightList = nullptr
	) override;
	void updateRenderQueueSplitOptions() override;
	void updateRenderQueueGroupSplitOptions(
		RenderQueueGroup* group, bool suppressShadows, bool suppressRenderState
	) override;
	Camera* createCamera(const String& name) override;
	Camera* getCamera(const String& name) const override;
	void destroyCamera(Camera* cam) override;
	void destroyCamera(const String& name) override;
	void _populateLightList(const Vector3& position, Real radius, LightList& destList) override;
	SceneNode* createSceneNode() override;
	SceneNode* createSceneNode(const String& name) override;
	void destroySceneNode(const String& name) override;
	SceneNode* getSceneNode(const String& name) const override;
	size_t estimateWorldGeometry(const String& filename) override { return 0; }
	size_t estimateWorldGeometry(DataStreamPtr& stream, const String& typeName = StringUtil::BLANK) override
	{
		return 0;
	}
	bool setOption(const String& strKey, const void* pValue) override { return false; }
	bool getOption(const String& strKey, void* pDestValue) override { return false; }
	bool hasOption(const String& strKey) const override { return false; }
	bool getOptionValues(const String& strKey, StringVector& refValueList) override { return false; }
	bool getOptionKeys(StringVector& refKeys) override { return false; }
	void _setDestinationRenderSystem(RenderSystem* sys) override;
	FogMode getFogMode() const override;
	void setShadowCameraSetup(const ShadowCameraSetupPtr& shadowSetup) override;
	Entity* createEntity(const String& entityName, const String& meshName) override;
	ParticleSystem* createParticleSystem(const String& name, const String& templateName) override;
	ParticleSystem* createParticleSystem(
		const String& name,
		size_t quota = 500,
		const String& resourceGroup = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
	) override;
	void setWorldGeometry(const String& filename) override;
	void setWorldGeometry(DataStreamPtr& stream, const String& typeName = StringUtil::BLANK) override;
	ViewPoint getSuggestedViewpoint(bool random = false) override;
	void _applySceneAnimations() override;
	void _renderScene(Camera* camera, Viewport* vp, bool includeOverlays) override;
	void _queueSkiesForRendering(Camera* cam) override;
	void setSkyPlane(
		bool enable,
		const Plane& plane,
		const String& materialName,
		Real scale = 1000,
		Real tiling = 10,
		bool drawFirst = true,
		Real bow = 0,
		int xsegments = 1,
		int ysegments = 1,
		const String& groupName = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
	) override;
	void setSkyBox(
		bool enable,
		const String& materialName,
		Real distance = 5000,
		bool drawFirst = true,
		const Quaternion& orientation = Quaternion::IDENTITY,
		const String& groupName = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
	) override;
	void setSkyDome(
		bool enable,
		const String& materialName,
		Real curvature = 10,
		Real tiling = 8,
		Real distance = 4000,
		bool drawFirst = true,
		const Quaternion& orientation = Quaternion::IDENTITY,
		int xsegments = 16,
		int ysegments = 16,
		int ysegments_keep = -1,
		const String& groupName = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
	) override;
	BillboardSet* createBillboardSet(const String& name, unsigned int poolSize = 20) override;
	Animation* createAnimation(const String& name, Real length) override;
	Animation* getAnimation(const String& name) const override;
	bool hasAnimation(const String& name) const override;
	void destroyAnimation(const String& name) override;
	void destroyAllAnimations() override;
	AnimationState* createAnimationState(const String& animName) override;
	void manualRender(
		RenderOperation* rend,
		Pass* pass,
		Viewport* vp,
		const Matrix4& worldMatrix,
		const Matrix4& viewMatrix,
		const Matrix4& projMatrix,
		bool doBeginEndFrame = false
	) override;
	void removeRenderQueueListener(RenderQueueListener* delListener) override;
	bool isRenderQueueToBeProcessed(uint8 qid) override;
	void setShadowTechnique(ShadowTechnique technique) override;
	void setShadowColour(const ColourValue& colour) override;
	void setShadowIndexBufferSize(size_t size) override;
	void setShadowTextureSize(unsigned short size) override;
	void setShadowTextureConfig(size_t shadowIndex, const ShadowTextureConfig& config) override;
	void setShadowTexturePixelFormat(PixelFormat fmt) override;
	void setShadowTextureCount(size_t count) override;
	size_t getShadowTextureCount() const { return mShadowTextureConfigList.size(); }
	void setShadowTextureSettings(unsigned short size, unsigned short count, PixelFormat fmt = PF_X8R8G8B8) override;
	const TexturePtr& getShadowTexture(size_t shadowIndex) override;
	void setShadowTextureCasterMaterial(const String& name) override;
	void setShadowTextureReceiverMaterial(const String& name) override;
	void removeShadowListener(ShadowListener* s) override;
	StaticGeometry* createStaticGeometry(const String& name) override;
	StaticGeometry* getStaticGeometry(const String& name) const override;
	void destroyStaticGeometry(const String& name) override;
	void destroyAllStaticGeometry() override;
	InstancedGeometry* createInstancedGeometry(const String& name) override;
	InstancedGeometry* getInstancedGeometry(const String& name) const override;
	void destroyInstancedGeometry(const String& name) override;
	void destroyAllInstancedGeometry() override;
	MovableObject* createMovableObject(
		const String& name, const String& typeName, const NameValuePairList* params = nullptr
	) override;
	void destroyMovableObject(const String& name, const String& typeName) override;
	void destroyAllMovableObjectsByType(const String& typeName) override;
	void destroyAllMovableObjects() override;
	MovableObject* getMovableObject(const String& name, const String& typeName) const override;
	bool hasMovableObject(const String& name, const String& typeName) const override;
	void extractMovableObject(const String& name, const String& typeName) override;
	MovableObjectIterator getMovableObjectIterator(const String& typeName) override;
	const Pass* _setPass(const Pass* pass, bool evenIfSuppressed = false, bool shadowDerivation = true) override;
	void _renderQueueGroupObjects(RenderQueueGroup* group, QueuedRenderableCollection::OrganisationMode om) override;
};

class RbxSceneManagerFactory : public SceneManagerFactory
{
protected:
	void initMetaData() const override;

public:
	RbxSceneManagerFactory() {}
	~RbxSceneManagerFactory() override = default;

	static const String FACTORY_TYPE_NAME;

	SceneManager* createInstance(const String& instanceName) override;
	void destroyInstance(SceneManager* instance) override;
};

} // namespace Ogre

#endif // APPDRAW_RBXSCENEMANAGER_H
