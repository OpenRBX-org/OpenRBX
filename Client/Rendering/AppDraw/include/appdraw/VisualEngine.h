#ifndef APPDRAW_VISUALENGINE_H
#define APPDRAW_VISUALENGINE_H

#include "appdraw/TwoDManager.h"

#include <OgreAxisAlignedBox.h>
#include <OgreEdgeListBuilder.h>
#include <OgreMesh.h>
#include <OgreRenderWindow.h>
#include <OgreRoot.h>
#include <OgreSubMesh.h>
#include <string>

class CRobloxWnd;
class RobloxWindow;

namespace G3D {
class Color4;
class Vector3;
} // namespace G3D

namespace RBX {
class AdornRbxGfx;

namespace RenderNew {
class Mesh;
class RenderScene;
class TextureProxy;
} // namespace RenderNew

namespace ViewNew {
class View;
} // namespace ViewNew
} // namespace RBX

namespace Ogre {

class RbxSubMesh : public SubMesh
{
public:
	RbxSubMesh();
	~RbxSubMesh();

	void buildEdgeList();

	bool m_edgeListBuilt;
	EdgeData* m_edgeList;
	AxisAlignedBox m_bounds;
	float m_boundingRadius;
};

Vector3 toOgreVector3(const G3D::Vector3& v);
ColourValue toOgreColour(const G3D::Color4& color);

class VisualEngine
{
public:
	class FrameRateManager
	{
	public:
		struct FrameStats
		{
			FrameStats()
				: m_frameTime(-1.0f), m_visibleCount(-1.0f), m_renderedCount(-1.0f), m_renderWindow(-1.0f),
				  m_viewport(-1.0f)
			{
			}

			float m_frameTime;
			float m_visibleCount;
			float m_renderedCount;
			float m_renderWindow;
			float m_viewport;
		};

		FrameRateManager();

		int getFrameCount() const;
		void addFrame(const FrameStats& entry);
		FrameStats& getFrame(int back);
		float getMedianFrameTime(int count);

	private:
		int m_frameCount;
		FrameStats m_frames[100];
	};

	VisualEngine();
	virtual ~VisualEngine();

	SceneManager* getSceneManager() const { return m_sceneManager; }
	Viewport* getViewport() const { return m_viewport; }

	FrameRateManager& getFrameRateManager() { return m_frameRateManager; }

	SubMesh* createSubMesh(
		MeshPtr& mesh, const unsigned short* indices16, const unsigned int* indices32, size_t triangleCount
	);

	VertexData* createVertexData(
		const float* positions,
		size_t vertexCount,
		const float* normals,
		const ColourValue* colours,
		const float* textureCoords
	);

	void setMeshVertexData(
		MeshPtr& mesh,
		const float* positions,
		size_t vertexCount,
		const float* normals,
		const ColourValue* colours,
		const float* textureCoords,
		const Vector3* minimum,
		const Vector3* maximum
	);

	MeshPtr createMesh(
		const char* name,
		const char* group,
		const float* positions,
		size_t vertexCount,
		const float* normals,
		const ColourValue* colours,
		const float* textureCoords,
		const Vector3* minimum,
		const Vector3* maximum
	);

	RbxSubMesh* createRbxSubMesh(
		const float* positions,
		size_t vertexCount,
		const float* normals,
		const ColourValue* colours,
		const float* textureCoords,
		const unsigned short* indices16,
		const unsigned int* indices32,
		size_t triangleCount,
		Vector3* minimumOut,
		Vector3* maximumOut
	);

	void initialise(
		int renderSystem, const String& window, int width, int height, int antialiasing, std::string logFileName
	);

private:
	friend class ::CRobloxWnd;
	friend class ::RobloxWindow;
	friend class RBX::AdornRbxGfx;
	friend class RBX::RenderNew::Mesh;
	friend class RBX::RenderNew::RenderScene;
	friend class RBX::RenderNew::TextureProxy;
	friend class RBX::ViewNew::View;
	friend class RbxFont;

	SceneManager* m_sceneManager;
	Camera* m_camera;
	RenderWindow* m_renderWindow;
	Viewport* m_viewport;
	TwoDManager* m_twoDManager;
	FrameRateManager m_frameRateManager;
	unsigned int m_videoMemorySize;
	bool m_use32BitIndices;
	bool m_useLargeVertexBuffer;
	int m_largeVertexBufferOverride;
	int m_depthBits;
	int m_antialiasing;
	bool m_softwareVertexProcessing;
	bool m_textureSquareOnly;
	bool m_supportDepthBias;
};

extern bool frameRateManagerEnabled;
extern Root* ogreRoot;

unsigned int getVideoMemorySize();
void destroyRoot();

inline void RbxSubMesh::buildEdgeList()
{
	if (!m_edgeListBuilt) {
		vertexData->prepareForShadowVolume();
		m_edgeListBuilt = true;
		EdgeListBuilder builder;
		builder.addVertexData(vertexData);
		builder.addIndexData(indexData, 0, operationType);
		m_edgeList = builder.build();
	}
}

inline RbxSubMesh::~RbxSubMesh()
{
	delete m_edgeList;
}

} // namespace Ogre

#endif // APPDRAW_VISUALENGINE_H
