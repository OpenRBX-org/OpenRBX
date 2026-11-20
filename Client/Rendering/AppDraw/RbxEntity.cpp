#include "appdraw/RbxEntity.h"

#include "appdraw/RbxSceneNode.h"
#include "appdraw/RbxSubEntity.h"

#include <OgreCamera.h>
#include <OgreLight.h>
#include <OgreLogManager.h>
#include <OgreMath.h>
#include <OgreMesh.h>
#include <OgrePass.h>
#include <OgrePlane.h>
#include <OgreRay.h>
#include <OgreRenderQueue.h>
#include <OgreStringConverter.h>
#include <OgreSubMesh.h>
#include <OgreTechnique.h>
#include <algorithm>
#include <cstring>

namespace Ogre {

void RbxEntity::setFrameRateManager(VisualEngine::FrameRateManager* frameRateManager)
{
	m_frameRateManager = frameRateManager;
}

void RbxEntity::setMesh(MeshPtr mesh)
{
	mMesh = mesh;
}

void RbxEntity::_notifyCurrentCamera(Camera* cam)
{
	if (!m_visibilityUpdated) {
		updateVisibility(cam);
		m_visibilityUpdated = true;
	}
}

void RbxEntity::setSubEntitiesVisible(bool visible)
{
	for (int i = 0; i < (int) mSubEntityList.size(); i++) {
		mSubEntityList[i]->setVisible(visible);
	}
}

void RbxEntity::_updateRenderQueue(RenderQueue* queue)
{
	if (!mInitialised) {
		return;
	}

	if (mMeshLodIndex > 0 && mMesh->isLodManual()) {
		mLodEntityList[mMeshLodIndex - 1]->_updateRenderQueue(queue);
		return;
	}

	SubEntityList::iterator i, iend;
	iend = mSubEntityList.end();
	for (i = mSubEntityList.begin(); i != iend; ++i) {
		if ((*i)->isVisible()) {
			if (mRenderQueueIDSet) {
				queue->addRenderable(*i, mRenderQueueID);
			}
			else {
				queue->addRenderable(*i);
			}
		}
	}
}

void RbxEntity::removeSubEntity(RbxSubEntity* subEntity)
{
	SubEntityList::iterator it;
	for (it = mSubEntityList.begin(); it != mSubEntityList.end(); ++it) {
		if (*it == subEntity) {
			mSubEntityList.erase(it);
			return;
		}
	}

	if (it == mSubEntityList.end()) {
		delete subEntity;
	}
}

RbxEntity::RbxEntity()
{
	m_minVisiblePercent = 20.0f;
	mInitialised = true;
	m_frustumCull = true;
	m_visibilityUpdated = false;
	m_recordFrameStats = true;
	m_frameRateManager = nullptr;
	m_minVisibleCount = 888;
}

void RbxEntity::clearSubEntities()
{
	mSubEntityList.clear();
}

RbxEntity::~RbxEntity()
{
	for (int i = 0; i < (int) getNumSubEntities(); i++) {
		RbxSubEntity* subEntity = dynamic_cast<RbxSubEntity*>(mSubEntityList[i]);
		if (subEntity) {
			delete subEntity;
		}
	}
	mSubEntityList.clear();
}

RbxSubEntity* RbxEntity::createSubEntity(SubMesh* subMesh, int lodIndex)
{
	RbxSubEntity* subEntity = new RbxSubEntity(this, subMesh, lodIndex);
	if (subMesh->isMatInitialised()) {
		subEntity->setMaterialName(subMesh->getMaterialName());
	}
	mSubEntityList.push_back(subEntity);
	return subEntity;
}

void RbxEntity::addSubEntity(RbxSubEntity* subEntity)
{
	mSubEntityList.push_back(subEntity);
}

void RbxEntity::updateVisibility(Camera* cam)
{
	RbxSceneNode* node = dynamic_cast<RbxSceneNode*>(getParentNode());
	std::vector<Candidate> candidates;

	bool managed;
	if (!frameRateManagerEnabled || !m_frameRateManager) {
		managed = false;
	}
	else {
		managed = true;
		candidates.resize(mSubEntityList.size(), Candidate());
		candidates.erase(candidates.begin(), candidates.end());
	}

	if (!mSubEntityList.empty()) {
		m_subEntityStates.resize(mSubEntityList.size());
		memset(&m_subEntityStates[0], 0, m_subEntityStates.size());
	}

	for (int i = 0; i < (int) mSubEntityList.size(); i++) {
		RbxSubEntity* subEntity = dynamic_cast<RbxSubEntity*>(mSubEntityList[i]);
		Vector3 offset = subEntity->m_worldTransform.getTrans();
		bool visible = true;

		SubMesh* subMesh = subEntity->getSubMesh();
		if (!subMesh) {
			subMesh = subEntity->getLodSubMesh(1);
		}

		Sphere sphere = node->getSubMeshSphere(subMesh);
		sphere.setCenter(sphere.getCenter() + offset);

		if (m_frustumCull) {
			visible = cam->isVisible(sphere, nullptr);
		}
		subEntity->setVisible(visible);

		if (!visible) {
			m_subEntityStates[i] = 2;
		}
		else {
			m_subEntityStates[i] = 1;
			if (managed) {
				Candidate candidate;
				candidate.m_index = i;
				candidate.m_priority = Math::Sqrt(subEntity->getSquaredViewDepth(cam)) - sphere.getRadius() -
									   subEntity->getRenderImportance() * 1000000.0;
				candidates.push_back(candidate);
			}
		}
	}

	if (managed) {
		int count = candidates.size();
		int mode = -1;

		if (m_frameRateManager->getFrameCount() > 30) {
			VisualEngine::FrameRateManager::FrameStats& last = m_frameRateManager->getFrame(0);
			Real average = m_frameRateManager->getMedianFrameTime(20);

			if (!(Math::Abs((average - last.m_frameTime) / average) <= 0.2)) {
				if (last.m_renderedCount < last.m_visibleCount) {
					Real limit = (Real) candidates.size() < last.m_renderedCount ? (Real) candidates.size()
																				 : last.m_renderedCount;
					count = limit;
				}
				else {
					count = candidates.size();
				}
				mode = 1;
			}
			else if (average > 50.0) {
				Real budget = m_minVisiblePercent;
				int minimum = budget / 100.0;
				Real fraction = minimum;
				if (average > 100.0) {
					count = last.m_renderedCount * fraction;
				}
				else {
					Real scale = 1.0f - (1.0f - fraction) * Math::Sin((average - 50.0) * Math::HALF_PI / 50.0);
					count = last.m_renderedCount * scale;
				}

				if (count < mSubEntityList.size() * fraction) {
					count = mSubEntityList.size() * fraction;
				}
				if (Math::Abs(last.m_renderedCount - count) < 40.0f) {
					count = last.m_renderedCount;
				}
				if (count < m_minVisibleCount) {
					count = m_minVisibleCount;
				}
				if (count > (int) candidates.size()) {
					count = candidates.size();
				}
				mode = 2;
			}
			else if (last.m_visibleCount > last.m_renderedCount) {
				count = last.m_renderedCount;
				Real growth = (50.0 - average) / average;
				if (growth > 1.2f) {
					growth = 1.2f;
				}
				int grown = count * growth;
				if (grown >= count + 40) {
					count = grown;
				}
				if (count > (int) candidates.size()) {
					count = candidates.size();
				}
				mode = 3;
			}
			else {
				count = candidates.size();
				mode = 4;
			}
		}

		if (count < (int) candidates.size()) {
			std::sort(candidates.begin(), candidates.end());
			for (int i = count; i < (int) candidates.size(); i++) {
				dynamic_cast<RbxSubEntity*>(mSubEntityList[candidates[i].m_index])->setVisible(false);
				m_subEntityStates[candidates[i].m_index] = 3;
			}

			LogManager::getSingleton().logMessage(
				StringConverter::toString(count) + "/" + StringConverter::toString(candidates.size()) +
				" p=" + StringConverter::toString(mode)
			);
		}

		if (m_recordFrameStats) {
			m_frameRateManager->addFrame(VisualEngine::FrameRateManager::FrameStats());
			VisualEngine::FrameRateManager::FrameStats& latest = m_frameRateManager->getFrame(0);
			latest.m_visibleCount = candidates.size();
			latest.m_renderedCount = count;
		}
	}
}

void RbxEntity::findShadowCasters(Camera* camera, const Light* light, std::vector<ShadowCaster*>* casters)
{
	casters->clear();
	if (!m_visibilityUpdated) {
		updateVisibility(camera);
		m_visibilityUpdated = true;
	}

	const Vector3* corners = m_bounds.getAllCorners();
	for (int i = 0; i < 8; i++) {
		if (camera->isVisible(corners[i])) {
			corners[i].distance(camera->getDerivedPosition());
		}
	}

	Real farDistance;
	{
		std::vector<Plane> planes;
		planes.push_back(Plane(Vector3(1.0f, 0.0f, 0.0f), Vector3(m_bounds.getMaximum().x, 0.0f, 0.0f)));
		planes.push_back(Plane(Vector3(-1.0f, 0.0f, 0.0f), Vector3(m_bounds.getMinimum().x, 0.0f, 0.0f)));
		planes.push_back(Plane(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, m_bounds.getMaximum().y, 0.0f)));
		planes.push_back(Plane(Vector3(0.0f, -1.0f, 0.0f), Vector3(0.0f, m_bounds.getMinimum().y, 0.0f)));
		planes.push_back(Plane(Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 0.0f, m_bounds.getMaximum().z)));
		planes.push_back(Plane(Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.0f, m_bounds.getMinimum().z)));

		const Vector3* frustumCorners = camera->getWorldSpaceCorners();
		Ray rays[4];
		for (int i = 0; i < 4; i++) {
			rays[i].setOrigin(frustumCorners[i]);
			Vector3 direction = frustumCorners[i + 4] - frustumCorners[i];
			direction.normalise();
			rays[i].setDirection(direction);
		}

		Real maxDistance = -1.0f;
		for (int i = 0; i < 4; i++) {
			Real minDistance = 1000000000.0f;
			for (unsigned int j = 0; j < 6; j++) {
				if (planes[j].getSide(rays[i].getOrigin()) == Plane::NEGATIVE_SIDE) {
					std::pair<bool, Real> hit = Math::intersects(rays[i], planes[j]);
					if (hit.first) {
						minDistance = std::min(minDistance, hit.second);
					}
				}
			}
			if (minDistance != 1000000000.0 && minDistance < 10000.0f) {
				maxDistance = std::max(minDistance, maxDistance);
			}
		}

		farDistance = camera->getNearClipDistance() + maxDistance;
	}

	Real savedFarDistance;
	if (farDistance > 0.0f) {
		savedFarDistance = camera->getFarClipDistance();
		camera->setFarClipDistance(farDistance);
	}

	bool lightInFrustum;
	const PlaneBoundedVolumeList* volList;
	if (light->getType() == Light::LT_DIRECTIONAL) {
		AxisAlignedBox aabb;
		const Vector3* frustumCorners = camera->getWorldSpaceCorners();
		Vector3 min, max;
		Vector3 extrude = light->getDerivedDirection() * -10000.0;

		min = max = frustumCorners[0];
		min.makeFloor(frustumCorners[0] + extrude);
		max.makeCeil(frustumCorners[0] + extrude);
		for (size_t c = 1; c < 8; ++c) {
			min.makeFloor(frustumCorners[c]);
			max.makeCeil(frustumCorners[c]);
			min.makeFloor(frustumCorners[c] + extrude);
			max.makeCeil(frustumCorners[c] + extrude);
		}
		aabb.setExtents(min, max);

		lightInFrustum = false;
		volList = &light->_getFrustumClipVolumes(camera);
	}
	else {
		Sphere s(light->getDerivedPosition(), light->getAttenuationRange());
		if (camera->isVisible(s)) {
			lightInFrustum = camera->isVisible(light->getDerivedPosition());
			if (!lightInFrustum) {
				volList = &light->_getFrustumClipVolumes(camera);
			}
			else {
				volList = nullptr;
			}
		}
	}

	for (int i = 0; i < (int) getNumSubEntities(); i++) {
		RbxSubEntity* subEntity = dynamic_cast<RbxSubEntity*>(mSubEntityList[i]);
		if (subEntity->getCastShadows()) {
			if (!subEntity->getMaterial()->getTechnique(0)->getPass(0)->isTransparent()) {
				if (subEntity->isVisible()) {
					casters->push_back(subEntity);
				}
				else if (m_subEntityStates[i] != 3 && (!lightInFrustum || light->getType() == Light::LT_DIRECTIONAL)) {
					PlaneBoundedVolumeList::const_iterator pi, piend;
					piend = volList->end();
					for (pi = volList->begin(); pi != piend; ++pi) {
						if (pi->intersects(subEntity->getWorldBoundingBox())) {
							casters->push_back(subEntity);
							break;
						}
					}
				}
			}
		}
	}

	if (farDistance > 0.0f) {
		camera->setFarClipDistance(savedFarDistance);
	}
}

String RbxEntityFactory::FACTORY_TYPE_NAME = "Entity";

} // namespace Ogre
