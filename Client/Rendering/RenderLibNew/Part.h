#ifndef RENDERLIBNEW_PART_H
#define RENDERLIBNEW_PART_H

#include "renderlibnew/Chunk.h"
#include "renderlibnew/Material.h"
#include "renderlibnew/Mesh.h"
#include "util/Events.h"
#include "util/Signals.h"
#include "v8world/IMoving.h"

#include <G3D/CoordinateFrame.h>
#include <G3D/ReferenceCount.h>
#include <G3D/Vector3.h>
#include <memory>

namespace RBX {

class Decal;
class Instance;
class PartInstance;
class SpecialShape;
class Texture;

namespace Reflection {
class PropertyDescriptor;
}

namespace ViewNew {

class View;

float primaryComponent(const G3D::Vector3& v);

class PartChunk : public RenderNew::Chunk
{
private:
	G3D::CoordinateFrame coordinateFrame;

	virtual void updateMesh() = 0;

	Signals::ScopedConnection childAddedConnection;
	Signals::ScopedConnection childRemovedConnection;
	Signals::ScopedConnection ancestorChangedConnection;
	Signals::ScopedConnection propertyChangedConnection;
	Signals::ScopedConnection shapePropertyChangedConnection;

public:
	virtual ~PartChunk();

	virtual bool castsShadows() const;
	virtual bool cullable() const;

protected:
	std::shared_ptr<PartInstance> partInstance;
	G3D::ReferenceCountedPointer<RenderNew::Material> material;
	bool materialInvalid;
	G3D::ReferenceCountedPointer<RenderNew::Mesh> mesh;
	View* view;
	SpecialShape* specialShape;

	PartChunk(float polygonOffset, const std::shared_ptr<PartInstance>& partInstance, View* view);

	void invalidateMaterial();
	void invalidateMesh();

	virtual void onPropertyChanged(const Reflection::PropertyDescriptor* descriptor);

	virtual G3D::ReferenceCountedPointer<RenderNew::Mesh> getMesh();
	virtual const G3D::CoordinateFrame& cframe();

private:
	void onAncestorChanged(std::shared_ptr<Instance> ancestor);
	void onChildAdded(std::shared_ptr<Instance> child);
	void onChildRemoved(std::shared_ptr<Instance> child);
	void onSpecialShapeChanged();
};

class Part : public PartChunk, public Listener<PartInstance, CanAggregateChanged>
{
public:
	Part(const std::shared_ptr<PartInstance>& partInstance, View* view);

	virtual ~Part();

	virtual G3D::ReferenceCountedPointer<RenderNew::Material> getMaterial();

protected:
	virtual void onPropertyChanged(const Reflection::PropertyDescriptor* descriptor);
	virtual void onEvent(const PartInstance* source, CanAggregateChanged event);

private:
	bool usesMegaTexture() const;

	virtual void updateMesh();
};

class Decal : public PartChunk
{
private:
	std::shared_ptr<RBX::Decal> decal;
	Signals::ScopedConnection decalAncestorChangedConnection;
	Signals::ScopedConnection decalPropertyChangedConnection;

public:
	Decal(RBX::Decal& decal, PartInstance& parentPart, View* view);

	virtual ~Decal();

	virtual G3D::ReferenceCountedPointer<RenderNew::Material> getMaterial();

protected:
	void onDecalPropertyChanged(const Reflection::PropertyDescriptor* descriptor);
	void onDecalAncestorChanged(std::shared_ptr<Instance> ancestor);

private:
	virtual void updateMesh();
};

class Texture : public PartChunk
{
private:
	std::shared_ptr<RBX::Texture> texture;
	Signals::ScopedConnection textureAncestorChangedConnection;
	Signals::ScopedConnection texturePropertyChangedConnection;

public:
	Texture(RBX::Texture& texture, PartInstance& parentPart, View* view);

	virtual ~Texture();

	virtual G3D::ReferenceCountedPointer<RenderNew::Material> getMaterial();

protected:
	void onTexturePropertyChanged(const Reflection::PropertyDescriptor* descriptor);
	void onTextureAncestorChanged(std::shared_ptr<Instance> ancestor);

private:
	virtual void updateMesh();
};

} // namespace ViewNew
} // namespace RBX

#endif // RENDERLIBNEW_PART_H
