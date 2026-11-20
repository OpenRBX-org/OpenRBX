#ifndef APPDRAW_TWODMANAGER_H
#define APPDRAW_TWODMANAGER_H

#include <G3D/Color4.h>
#include <OgreHardwareIndexBuffer.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreRenderOperation.h>
#include <OgreRenderQueueListener.h>
#include <list>
#include <string>

namespace Ogre {

class TwoDManager : public RenderQueueListener
{
public:
	struct Sprite
	{
		float x1;
		float y1;
		float x2;
		float y2;
		float tx1;
		float ty1;
		float tx2;
		float ty2;
		uint32 colour;
		int texHandle;
		int operationType;
		int segment;
	};

	struct VertexChunk
	{
		uint32 texHandle;
		int operationType;
		int segment;
		int vertexCount;
	};

	~TwoDManager() override;
	TwoDManager();

	void init(
		SceneManager* sceneManager, RenderTarget* target, uint8 queueGroupId, bool afterQueue, bool unitCoordinates
	);
	void prepareForRender();
	void createHardwareBuffer(size_t numVertices);
	void renderBuffer();
	void spriteBltFull(
		std::string texture,
		const G3D::Color4& color,
		float x0,
		float y0,
		float x1,
		float y1,
		float u0,
		float v0,
		float u1,
		float v1
	);
	void rectBlt(int type, float x0, float y0, float x1, float y1, const G3D::Color4& color);
	void lineBlt(float x0, float y0, float x1, float y1, const G3D::Color4& color);

	void renderQueueStarted(uint8 queueGroupId, const String& invocation, bool& skipThisInvocation) override;
	void renderQueueEnded(uint8 queueGroupId, const String& invocation, bool& repeatThisInvocation) override;

private:
	SceneManager* m_sceneManager;
	uint8 m_targetQueue;
	bool m_afterQueue;
	bool m_unitCoordinates;
	RenderTarget* m_renderTarget;
	RenderOperation m_renderOperation;
	HardwareVertexBufferSharedPtr m_hardwareBuffer;
	HardwareIndexBufferSharedPtr m_unk0x30;
	std::list<Sprite> m_sprites;
};

} // namespace Ogre

#endif // APPDRAW_TWODMANAGER_H
