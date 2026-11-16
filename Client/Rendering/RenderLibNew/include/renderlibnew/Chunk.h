#ifndef RENDERLIBNEW_CHUNK_H
#define RENDERLIBNEW_CHUNK_H

#include "renderlib/RenderStats.h"
#include "renderlibnew/Material.h"
#include "renderlibnew/Mesh.h"

#include <G3D/Array.h>
#include <G3D/CoordinateFrame.h>
#include <G3D/GLight.h>
#include <G3D/ReferenceCount.h>
#include <GLG3D/RenderDevice.h>
#include <vector>

namespace Ogre {
class RbxEntity;
class RbxSubEntity;
} // namespace Ogre

namespace RBX::RenderNew {

class AggregateChunk;

class Chunk : public G3D::ReferenceCountedObject
{
public:
	static const float DECAL_OFFSET;
	static const float TEXTURE_OFFSET;

	Ogre::RbxEntity* m_unk0x0c;
	Ogre::RbxSubEntity* m_unk0x10;
	float m_unk0x14;
	float polygonOffset;
	float radius;

	Chunk(float polygonOffset);

	~Chunk() override;

	virtual bool cachesShadows() const { return false; }

	virtual void renderShadows(G3D::RenderDevice* rd, const G3D::GLight& source, bool caps, float shadowVertexDistance)
	{
	}
	virtual bool castsShadows() const = 0;
	virtual bool cullable() const = 0;
	virtual G3D::ReferenceCountedPointer<Material> getMaterial() = 0;
	virtual G3D::ReferenceCountedPointer<Mesh> getMesh() = 0;
	virtual const G3D::CoordinateFrame& cframe() = 0;
	virtual void VTable0x20();

	virtual void VTable0x24(float p_unk0x14) { m_unk0x14 = p_unk0x14; }

	virtual float VTable0x28() { return m_unk0x14; }

	G3D::WeakReferenceCountedPointer<AggregateChunk> aggregate;
};

class AggregateChunk : public Chunk
{
private:
	G3D::ReferenceCountedPointer<Material> material;
	G3D::ReferenceCountedPointer<Mesh> mesh;
	G3D::CoordinateFrame _cframe;
	bool _castsShadows;
	bool _cullable;

public:
	static bool randomColors;
	static const G3D::CoordinateFrame identity;

	bool cachesShadows() const override { return true; }

	bool castsShadows() const override { return _castsShadows; }

	bool cullable() const override { return _cullable; }

	const G3D::CoordinateFrame& cframe() override { return _cframe; }

	G3D::ReferenceCountedPointer<Material> getMaterial() override { return material; }

	G3D::ReferenceCountedPointer<Mesh> getMesh() override { return mesh; }

	using Components = std::vector<G3D::ReferenceCountedPointer<Chunk>>;

	Components components;

	AggregateChunk(const G3D::ReferenceCountedPointer<Chunk>& firstChunk);
	~AggregateChunk() override;

	void makeMesh();
};

} // namespace RBX::RenderNew

#endif // RENDERLIBNEW_CHUNK_H
