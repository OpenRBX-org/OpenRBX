#ifndef RENDERLIBNEW_SPHEREMESH_H
#define RENDERLIBNEW_SPHEREMESH_H

#include "MeshFactory.h"
#include "renderlibnew/Mesh.h"
#include "util/NormalId.h"

#include <G3D/Vector2.h>
#include <G3D/Vector3.h>

namespace RBX::ViewNew {

class SphereMesh : public RenderNew::Mesh, public MeshFactory<SphereMesh, 1>
{
private:
	friend class MeshFactory<SphereMesh, 1>;

	SphereMesh(const G3D::Vector3& size, NormalId textureFace, const G3D::Vector2& studsPerTile);

	SphereMesh(const G3D::Vector3& size, NormalId decalFace);

	SphereMesh(const G3D::Vector3& diameter, RenderSurfaceTypes surfaceTypes);
};

} // namespace RBX::ViewNew

#endif // RENDERLIBNEW_SPHEREMESH_H
