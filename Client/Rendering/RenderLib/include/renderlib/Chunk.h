#ifndef RENDERLIB_CHUNK_H
#define RENDERLIB_CHUNK_H

#include "renderlib/Material.h"
#include "renderlib/Mesh.h"
#include "renderlib/RenderStats.h"

#include <G3D/Array.h>
#include <G3D/CoordinateFrame.h>
#include <G3D/GLight.h>
#include <G3D/ReferenceCount.h>
#include <GLG3D/RenderDevice.h>
#include <GLG3D/VAR.h>
#include <vector>

namespace RBX::Render {

class AggregateChunk;

class __declspec(novtable) Chunk : public G3D::ReferenceCountedObject
{
public:
	static const float DECAL_OFFSET;
	static const float TEXTURE_OFFSET;

	float polygonOffset;
	float radius;

	Chunk(float polygonOffset);

	~Chunk() override { RenderStats::chunkCount--; }

	virtual bool cachesShadows() const { return false; }

	virtual void renderShadows(G3D::RenderDevice* rd, const G3D::GLight& source, bool caps, float shadowVertexDistance)
	{
	}
	virtual bool castsShadows() const = 0;
	virtual bool cullable() const = 0;
	virtual G3D::ReferenceCountedPointer<Material> getMaterial() = 0;
	virtual G3D::ReferenceCountedPointer<Mesh> getMesh() = 0;
	virtual const G3D::CoordinateFrame& cframe() = 0;

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
	G3D::GLight shadowSource;
	G3D::VAR shadowVAR;
	G3D::Array<unsigned int> shadowIndexArray;
	G3D::Array<unsigned short> shadowIndexArray16;

public:
	static bool randomColors;
	static const G3D::CoordinateFrame identity;

	bool cachesShadows() const override { return true; }

	bool castsShadows() const override { return _castsShadows; }

	bool cullable() const override { return _cullable; }

	const G3D::CoordinateFrame& cframe() override { return _cframe; }

	G3D::ReferenceCountedPointer<Material> getMaterial() override { return material; }

	G3D::ReferenceCountedPointer<Mesh> getMesh() override { return mesh; }

	void renderShadows(G3D::RenderDevice* rd, const G3D::GLight& light, bool caps, float shadowVertexDistance) override;

	using Components = std::vector<G3D::ReferenceCountedPointer<Chunk>>;

	Components components;

	AggregateChunk(const G3D::ReferenceCountedPointer<Chunk>& firstChunk);
	~AggregateChunk() override;

	void makeMesh();
};

} // namespace RBX::Render

#endif // RENDERLIB_CHUNK_H
