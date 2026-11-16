#ifndef RENDERLIB_AGGREGATINGSCENEMANAGER_H
#define RENDERLIB_AGGREGATINGSCENEMANAGER_H

#include "renderlib/Chunk.h"
#include "renderlib/Material.h"
#include "renderlib/RenderScene.h"

#include <G3D/ReferenceCount.h>
#include <map>
#include <set>
#include <vector>

namespace RBX::Render {

class AggregatingSceneManager : public SceneManager
{
public:
	struct BucketKey
	{
		bool castsShadows;
		bool cullable;
		float polygonOffset;
		G3D::ReferenceCountedPointer<Material> material;

		BucketKey(const G3D::ReferenceCountedPointer<Chunk>& chunk)
			: castsShadows(chunk->castsShadows()), cullable(chunk->cullable()), polygonOffset(chunk->polygonOffset),
			  material(chunk->getMaterial())
		{
		}

		bool operator<(const BucketKey& that) const;
	};

	class Bucket : public G3D::ReferenceCountedObject
	{
	public:
		Bucket() = default;
		Bucket(const Bucket&) = delete;
		Bucket& operator=(const Bucket&) = delete;

		bool dequeueSleepingChunk(const G3D::ReferenceCountedPointer<Chunk>& chunk);
		void addToQueue(const G3D::ReferenceCountedPointer<Chunk>& chunk);
		void clear() { queue.clear(); }
		void optimize(AggregatingSceneManager& sceneManager, double maxTime);

		using Queue = std::vector<G3D::ReferenceCountedPointer<Chunk>>;

	private:
		Queue queue;

	public:
	};

	using BucketRef = G3D::ReferenceCountedPointer<Bucket>;

private:
	std::map<BucketKey, BucketRef> buckets;
	std::set<G3D::WeakReferenceCountedPointer<Chunk>> invalidSleepingModels;
	std::set<G3D::WeakReferenceCountedPointer<Chunk>> invalidMovingModels;

public:
	static unsigned int aggregationSize;

	AggregatingSceneManager(RenderScene* renderScene);
	~AggregatingSceneManager() override;

	void invalidateModel(const G3D::ReferenceCountedPointer<Chunk>& chunk, bool isSleeping) override;
	void addModel(const G3D::ReferenceCountedPointer<Chunk>& chunk) override;
	void removeModel(const G3D::ReferenceCountedPointer<Chunk>& chunk) override;
	void clear() override;
	void setSleeping(const G3D::ReferenceCountedPointer<Chunk>& chunk, bool sleeping) override;
	void prerender(double optimizationTime) override;

private:
	void dequeueSleepingChunk(const G3D::ReferenceCountedPointer<Chunk>& chunk);
	void queueSleepingChunk(const G3D::ReferenceCountedPointer<Chunk>& chunk);
	void deconstructAggregate(G3D::ReferenceCountedPointer<AggregateChunk> aggregate);

public:
};

} // namespace RBX::Render

#endif // RENDERLIB_AGGREGATINGSCENEMANAGER_H
