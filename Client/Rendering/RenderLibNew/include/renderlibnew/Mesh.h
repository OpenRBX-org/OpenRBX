#ifndef RENDERLIBNEW_MESH_H
#define RENDERLIBNEW_MESH_H

#include <G3D/Array.h>
#include <G3D/CoordinateFrame.h>
#include <G3D/MeshAlg.h>
#include <G3D/ReferenceCount.h>
#include <G3D/Vector2.h>
#include <G3D/Vector3.h>
#include <GLG3D/RenderDevice.h>
#include <GLG3D/VAR.h>
#include <OgreMesh.h>
#include <exception>
#include <stdexcept>
#include <vector>

class ThumbnailGenerator;

namespace Ogre {
class RbxSubMesh;
class VisualEngine;
} // namespace Ogre

namespace RBX::RenderNew {

class Chunk;
class RenderScene;

class Mesh : public G3D::ReferenceCountedObject
{
public:
	using IndexType = unsigned int;
	using VertexList = G3D::Array<int>;

	class Level : public G3D::ReferenceCountedObject
	{
	public:
		G3D::Array<IndexType> indexArray;
		G3D::RenderDevice::Primitive primitive;
		bool m_subMeshDirty;
		int m_sharedSubMeshIndex;
		Ogre::SubMesh* m_subMesh;

		Level();

		Level(G3D::RenderDevice::Primitive primitive)
			: primitive(primitive), m_subMeshDirty(true), m_sharedSubMeshIndex(-1), m_subMesh(nullptr)
		{
		}

		~Level() override;

		void releaseSubMesh();
		void makeHWSubMesh(Ogre::VisualEngine* engine, Ogre::AxisAlignedBox& box);
		void makeSubMesh(Ogre::VisualEngine* engine, Ogre::AxisAlignedBox& box);
	};

	using LevelRef = G3D::ReferenceCountedPointer<Level>;

	G3D::Vector3& getVertex(int index)
	{
		if (index < 0 || index >= visibleGeometry.vertexArray.size()) {
			throw std::runtime_error("RBX Mesh::getVertex() index out of range");
		}
		return visibleGeometry.vertexArray[index];
	}

	const G3D::Vector3& getNormal(int index)
	{
		if (index < 0 || index >= visibleGeometry.normalArray.size()) {
			throw std::runtime_error("RBX Mesh::getNormal() index out of range");
		}
		return visibleGeometry.normalArray[index];
	}

	static void setShadowVertex(const G3D::Vector3&);

protected:
	static G3D::MeshAlg::Geometry visibleGeometry;

private:
	friend class ::ThumbnailGenerator;
	friend class RenderScene;

	static G3D::Array<G3D::Vector2> texCoordArray;
	static G3D::Array<G3D::Vector3> tangentArray;
	static VertexList vertexRefCounts;
	static VertexList freeList;
	static G3D::VAR vertexVAR;
	static G3D::VAR normalVAR;
	static G3D::VAR texCoordVAR;
	static G3D::VAR tangentVAR;
	static bool varDirty;

	static void makeVAR();

	static G3D::Vector3 findScale;
	static VertexList findGrid[32][32][32];
	static G3D::Array<int>& getFindGridCell(const G3D::Vector3&);
	static unsigned int findVertex(
		const G3D::Array<int>&, const G3D::Vector3&, const G3D::Vector3&, const G3D::Vector2&
	);
	static void toFindGridCoords(const G3D::Vector3&, int&, int&, int&);

protected:
	static void initStatics();

	Mesh();

	G3D::Array<LevelRef> levels;

	int detailIndex(double detail) const { return G3D::iClamp((int) detail, 0, levels.size() - 1); }

	std::vector<unsigned short> m_shadowIndices;
	std::vector<G3D::Vector3> m_shadowVertices;
	Ogre::RbxSubMesh* m_shadowSubMesh;

public:
	int m_meshType;

	static Ogre::MeshPtr sharedMesh;

	static void updateSharedMesh(Ogre::VisualEngine* engine);
	void makeShadowSubMesh(Ogre::VisualEngine* engine);

	static G3D::ReferenceCountedPointer<Mesh> aggregate(
		const std::vector<G3D::ReferenceCountedPointer<Chunk>>& chunkArray,
		G3D::CoordinateFrame& outCFrame,
		float& outBoundingRadius
	);
	static unsigned int findVertex(const G3D::Vector3&, const G3D::Vector3&, const G3D::Vector2&);

	~Mesh() override;

	static void freeVertex(unsigned int index);

	static unsigned int allocVertex(unsigned int i, unsigned int count);

	static unsigned int allocVertex(
		const G3D::Vector3& vertex, const G3D::Vector3& normal, const G3D::Vector2& tex, bool share
	);

	static void beginRender(G3D::RenderDevice* rd, bool usetexCoords, bool useTangent);

	static void sendGeometry(const Level* lvl, G3D::RenderDevice* rd);

	static void endRender(G3D::RenderDevice* rd);

	const LevelRef detailLevel(float detail) const;

	static void flushVAR();
};

} // namespace RBX::RenderNew

#endif // RENDERLIBNEW_MESH_H
