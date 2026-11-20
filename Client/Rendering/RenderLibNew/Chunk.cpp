#include "renderlibnew/Chunk.h"

#include "appdraw/RbxEntity.h"
#include "renderlib/RenderStats.h"

#include <G3D/g3dmath.h>

namespace RBX::RenderNew {

const float Chunk::DECAL_OFFSET = -0.2f;
const float Chunk::TEXTURE_OFFSET = -0.1f;

bool AggregateChunk::randomColors;
const G3D::CoordinateFrame AggregateChunk::identity;

void Chunk::releaseEntity()
{
	if (m_subEntity) {
		m_entity->removeSubEntity(m_subEntity);
	}

	m_subEntity = nullptr;
	m_entity = nullptr;
}

void AggregateChunk::makeMesh()
{
	mesh = Mesh::aggregate(components, _cframe, radius);
}

Chunk::Chunk(float polygonOffset) : m_renderImportance(0.0f), polygonOffset(polygonOffset), radius(G3D::inf())
{
	RenderStats::chunkCount++;
	m_subEntity = nullptr;
	m_entity = nullptr;
}

Chunk::~Chunk()
{
	RenderStats::chunkCount--;

	if (m_subEntity) {
		m_entity->removeSubEntity(m_subEntity);
	}
}

AggregateChunk::AggregateChunk(const G3D::ReferenceCountedPointer<Chunk>& firstChunk)
	: Chunk(firstChunk->polygonOffset), _castsShadows(firstChunk->castsShadows()), _cullable(firstChunk->cullable())
{
	if (!randomColors) {
		material = firstChunk->getMaterial();
	}

	RenderStats::aggregateChunkCount++;
	RenderStats::chunkCount--;

	if (randomColors) {
		material = new Material();
		material->appendLevel(nullptr, (G3D::Color3::wheelRandom() + G3D::Color3::white()) / 2.0f, 0, 50, 0, 0);
	}
}

AggregateChunk::~AggregateChunk()
{
	RenderStats::chunkCount++;
	RenderStats::aggregateChunkCount--;
	RenderStats::aggregatedChunkCount -= components.size();
}

} // namespace RBX::RenderNew
