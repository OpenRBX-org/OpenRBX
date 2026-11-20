#include "appdraw/TwoDManager.h"

#include <OgreHardwareBufferManager.h>
#include <OgreLogManager.h>
#include <OgreRenderSystem.h>
#include <OgreRenderTarget.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreTextureManager.h>

namespace Ogre {

void TwoDManager::init(
	SceneManager* sceneManager, RenderTarget* target, uint8 queueGroupId, bool afterQueue, bool unitCoordinates
)
{
	m_afterQueue = afterQueue;
	m_targetQueue = queueGroupId;
	m_sceneManager = sceneManager;
	m_renderTarget = target;
	m_unitCoordinates = unitCoordinates;
	m_hardwareBuffer.setNull();
	sceneManager->addRenderQueueListener(this);
}

void TwoDManager::prepareForRender()
{
	LayerBlendModeEx colourBlend = LayerBlendModeEx();
	LayerBlendModeEx alphaBlend = LayerBlendModeEx();
	RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();

	colourBlend.blendType = LBT_COLOUR;
	colourBlend.operation = LBX_SOURCE1;
	colourBlend.source1 = LBS_TEXTURE;
	alphaBlend.blendType = LBT_ALPHA;
	alphaBlend.operation = LBX_SOURCE1;
	alphaBlend.source1 = LBS_TEXTURE;

	TextureUnitState::UVWAddressingMode addressingMode;
	addressingMode.u = TextureUnitState::TAM_CLAMP;
	addressingMode.v = TextureUnitState::TAM_CLAMP;
	addressingMode.w = TextureUnitState::TAM_CLAMP;

	renderSystem->_setWorldMatrix(Matrix4::IDENTITY);

	Matrix4 projection;
	if (m_unitCoordinates) {
		Matrix4 view = Matrix4::IDENTITY;
		view[0][3] = -0.5f;
		view[1][3] = -0.5f;
		view[2][3] = 0.0f;
		view[0][0] = 0.5f;
		view[1][1] = 0.5f;
		view[2][2] = 0.0f;
		renderSystem->_setViewMatrix(view);

		Radian fovy(Math::PI * 0.5);
		Real width = m_renderTarget->getWidth();
		renderSystem->_makeOrthoMatrix(fovy, width / m_renderTarget->getHeight(), 1.0f, 0.0f, projection, false);
	}
	else {
		Real nearPlane = m_renderTarget->getHeight() * 0.5;
		m_renderTarget->getHeight();

		Matrix4 view = Matrix4::IDENTITY;
		view[0][3] = m_renderTarget->getWidth() * -0.5;
		view[1][3] = m_renderTarget->getHeight() * -0.5;
		view[2][3] = 0.0f;
		renderSystem->_setViewMatrix(view);

		Radian fovy(Math::PI * 0.5);
		Real width = m_renderTarget->getWidth();
		renderSystem->_makeOrthoMatrix(fovy, width / m_renderTarget->getHeight(), nearPlane, 0.0f, projection, false);
	}

	renderSystem->_setProjectionMatrix(projection);
	renderSystem->_setTextureMatrix(0, Matrix4::IDENTITY);
	renderSystem->_setTextureCoordSet(0, 0);
	renderSystem->_setTextureCoordCalculation(0, TEXCALC_NONE, nullptr);
	renderSystem->_setTextureUnitFiltering(0, FO_LINEAR, FO_LINEAR, FO_LINEAR);
	renderSystem->_setTextureBlendMode(0, colourBlend);
	renderSystem->_setTextureBlendMode(0, alphaBlend);
	renderSystem->_setTextureAddressingMode(0, addressingMode);
	renderSystem->_disableTextureUnitsFrom(1);
	renderSystem->setLightingEnabled(false);
	renderSystem->_setFog(FOG_NONE, ColourValue::White, 1.0f, 0.0f, 1.0f);
	renderSystem->_setCullingMode(CULL_NONE);
	renderSystem->_setDepthBufferParams(false, false, CMPF_LESS_EQUAL);
	renderSystem->_setColourBufferWriteEnabled(true, true, true, false);
	renderSystem->setShadingType(SO_GOURAUD);
	renderSystem->_setPolygonMode(PM_SOLID);
	renderSystem->unbindGpuProgram(GPT_FRAGMENT_PROGRAM);
	renderSystem->unbindGpuProgram(GPT_VERTEX_PROGRAM);
	renderSystem->_setSceneBlending(SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA);
	renderSystem->_setAlphaRejectSettings(CMPF_ALWAYS_PASS, 0);
}

void TwoDManager::createHardwareBuffer(size_t numVertices)
{
	m_renderOperation.vertexData = new VertexData();
	m_renderOperation.vertexData->vertexStart = 0;

	VertexDeclaration* declaration = m_renderOperation.vertexData->vertexDeclaration;
	size_t offset = 0;
	declaration->addElement(0, offset, VET_FLOAT3, VES_POSITION);
	offset += VertexElement::getTypeSize(VET_FLOAT3);
	declaration->addElement(0, offset, VET_COLOUR, VES_DIFFUSE);
	offset += VertexElement::getTypeSize(VET_COLOUR);
	declaration->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);

	m_hardwareBuffer = HardwareBufferManager::getSingleton().createVertexBuffer(
		declaration->getVertexSize(0), numVertices, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE, false
	);
	m_renderOperation.vertexData->vertexBufferBinding->setBinding(0, m_hardwareBuffer);

	m_renderOperation.useIndexes = false;
	m_renderOperation.operationType = RenderOperation::OT_TRIANGLE_LIST;
}

TwoDManager::~TwoDManager() = default;

TwoDManager::TwoDManager() = default;

void TwoDManager::renderBuffer()
{
	RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();
	std::list<VertexChunk> batches;

	if (m_sprites.size() == 0) {
		return;
	}

	std::list<Sprite>::iterator end = m_sprites.end();
	std::list<Sprite>::iterator it;
	size_t numVertices = 0;
	for (it = m_sprites.begin(); it != end; ++it) {
		if (it->operationType == RenderOperation::OT_TRIANGLE_LIST) {
			numVertices += 6;
		}
		else if (it->segment == 0) {
			numVertices += 8;
		}
		else {
			numVertices += 2;
		}
	}

	if (numVertices < 120) {
		numVertices = 120;
	}

	if (m_hardwareBuffer.isNull() || m_hardwareBuffer->getNumVertices() < numVertices) {
		if (!m_hardwareBuffer.isNull()) {
			delete m_renderOperation.vertexData;
			m_renderOperation.vertexData = nullptr;
			m_hardwareBuffer.setNull();
		}

		createHardwareBuffer(numVertices);

		if (m_hardwareBuffer.isNull()) {
			LogManager::getSingleton().logMessage("TwoDManager: failed to create hardware buffer\n");
			return;
		}
	}

	float* vertex = static_cast<float*>(m_hardwareBuffer->lock(HardwareBuffer::HBL_DISCARD));
	uint32* colour;

	it = m_sprites.begin();
	VertexChunk batch;
	batch.texHandle = it->texHandle;
	batch.operationType = it->operationType;
	batch.segment = it->segment;
	batch.vertexCount = 0;

	while (it != end) {
		int count;

		if (batch.operationType == RenderOperation::OT_TRIANGLE_LIST) {
			*vertex++ = it->x1;
			*vertex++ = it->y2;
			*vertex++ = -1.0f;
			colour = reinterpret_cast<uint32*>(vertex);
			*colour++ = it->colour;
			vertex = reinterpret_cast<float*>(colour);
			*vertex++ = it->tx1;
			*vertex++ = it->ty2;

			*vertex++ = it->x2;
			*vertex++ = it->y1;
			*vertex++ = -1.0f;
			colour = reinterpret_cast<uint32*>(vertex);
			*colour++ = it->colour;
			vertex = reinterpret_cast<float*>(colour);
			*vertex++ = it->tx2;
			*vertex++ = it->ty1;

			*vertex++ = it->x1;
			*vertex++ = it->y1;
			*vertex++ = -1.0f;
			colour = reinterpret_cast<uint32*>(vertex);
			*colour++ = it->colour;
			vertex = reinterpret_cast<float*>(colour);
			*vertex++ = it->tx1;
			*vertex++ = it->ty1;

			*vertex++ = it->x1;
			*vertex++ = it->y2;
			*vertex++ = -1.0f;
			colour = reinterpret_cast<uint32*>(vertex);
			*colour++ = it->colour;
			vertex = reinterpret_cast<float*>(colour);
			*vertex++ = it->tx1;
			*vertex++ = it->ty2;

			*vertex++ = it->x2;
			*vertex++ = it->y1;
			*vertex++ = -1.0f;
			colour = reinterpret_cast<uint32*>(vertex);
			*colour++ = it->colour;
			vertex = reinterpret_cast<float*>(colour);
			*vertex++ = it->tx2;
			*vertex++ = it->ty1;

			*vertex++ = it->x2;
			*vertex++ = it->y2;
			*vertex++ = -1.0f;
			colour = reinterpret_cast<uint32*>(vertex);
			*colour++ = it->colour;
			vertex = reinterpret_cast<float*>(colour);
			*vertex++ = it->tx2;
			*vertex++ = it->ty2;

			count = 6;
		}
		else {
			*vertex++ = it->x1;
			*vertex++ = it->y1;
			*vertex++ = -1.0f;
			colour = reinterpret_cast<uint32*>(vertex);
			*colour++ = it->colour;
			vertex = reinterpret_cast<float*>(colour);
			*vertex++ = it->tx1;
			*vertex++ = it->ty1;

			if (batch.segment == 0) {
				*vertex++ = it->x2;
				*vertex++ = it->y1;
				*vertex++ = -1.0f;
				colour = reinterpret_cast<uint32*>(vertex);
				*colour++ = it->colour;
				vertex = reinterpret_cast<float*>(colour);
				*vertex++ = it->tx1;
				*vertex++ = it->ty1;

				*vertex++ = it->x2;
				*vertex++ = it->y1;
				*vertex++ = -1.0f;
				colour = reinterpret_cast<uint32*>(vertex);
				*colour++ = it->colour;
				vertex = reinterpret_cast<float*>(colour);
				*vertex++ = it->tx1;
				*vertex++ = it->ty1;

				*vertex++ = it->x2;
				*vertex++ = it->y2;
				*vertex++ = -1.0f;
				colour = reinterpret_cast<uint32*>(vertex);
				*colour++ = it->colour;
				vertex = reinterpret_cast<float*>(colour);
				*vertex++ = it->tx1;
				*vertex++ = it->ty1;

				*vertex++ = it->x2;
				*vertex++ = it->y2;
				*vertex++ = -1.0f;
				colour = reinterpret_cast<uint32*>(vertex);
				*colour++ = it->colour;
				vertex = reinterpret_cast<float*>(colour);
				*vertex++ = it->tx1;
				*vertex++ = it->ty1;

				*vertex++ = it->x1;
				*vertex++ = it->y2;
				*vertex++ = -1.0f;
				colour = reinterpret_cast<uint32*>(vertex);
				*colour++ = it->colour;
				vertex = reinterpret_cast<float*>(colour);
				*vertex++ = it->tx1;
				*vertex++ = it->ty1;

				*vertex++ = it->x1;
				*vertex++ = it->y2;
				*vertex++ = -1.0f;
				colour = reinterpret_cast<uint32*>(vertex);
				*colour++ = it->colour;
				vertex = reinterpret_cast<float*>(colour);
				*vertex++ = it->tx1;
				*vertex++ = it->ty1;

				*vertex++ = it->x1;
				*vertex++ = it->y1;
				*vertex++ = -1.0f;
				colour = reinterpret_cast<uint32*>(vertex);
				*colour++ = it->colour;
				vertex = reinterpret_cast<float*>(colour);
				*vertex++ = it->tx1;
				*vertex++ = it->ty1;

				count = 8;
			}
			else {
				*vertex++ = it->x2;
				*vertex++ = it->y2;
				*vertex++ = -1.0f;
				colour = reinterpret_cast<uint32*>(vertex);
				*colour++ = it->colour;
				vertex = reinterpret_cast<float*>(colour);
				*vertex++ = it->tx1;
				*vertex++ = it->ty1;

				count = 2;
			}
		}

		batch.vertexCount += count;
		++it;

		if (it == end || it->texHandle != batch.texHandle || it->operationType != batch.operationType) {
			batches.push_back(batch);

			if (it != end) {
				batch.texHandle = it->texHandle;
				batch.operationType = it->operationType;
				batch.segment = it->segment;
				batch.vertexCount = 0;
			}
		}
	}

	m_hardwareBuffer->unlock();
	prepareForRender();

	TexturePtr texture;
	m_renderOperation.vertexData->vertexStart = 0;
	TexturePtr current;

	std::list<VertexChunk>::iterator batchEnd = batches.end();
	for (std::list<VertexChunk>::iterator batchIt = batches.begin(); batchIt != batchEnd; ++batchIt) {
		if (batchIt->texHandle == 0) {
			renderSystem->_disableTextureUnit(0);
			current.setNull();
		}
		else {
			texture = TextureManager::getSingleton().getByHandle(batchIt->texHandle);

			if (texture.get() != current.get()) {
				renderSystem->_setTexture(0, true, texture);
				renderSystem->_setTextureMatrix(0, Matrix4::IDENTITY);

				LayerBlendModeEx colourBlend = LayerBlendModeEx();
				LayerBlendModeEx alphaBlend = LayerBlendModeEx();
				colourBlend.blendType = LBT_COLOUR;
				colourBlend.operation = LBX_SOURCE1;
				if (texture->getName() == "ft0" || texture->getFormat() == PF_A8) {
					colourBlend.source1 = LBS_DIFFUSE;
				}
				else {
					colourBlend.source1 = LBS_TEXTURE;
					colourBlend.source2 = LBS_CURRENT;
				}
				current = texture;

				alphaBlend.blendType = LBT_ALPHA;
				alphaBlend.source1 = LBS_TEXTURE;
				alphaBlend.source2 = LBS_DIFFUSE;
				alphaBlend.operation = LBX_SOURCE1;

				renderSystem->_setTextureBlendMode(0, colourBlend);
				renderSystem->_setTextureBlendMode(0, alphaBlend);
				renderSystem->_setTextureUnitFiltering(0, FO_LINEAR, FO_LINEAR, FO_LINEAR);
			}
		}

		m_renderOperation.vertexData->vertexCount = batchIt->vertexCount;
		m_renderOperation.operationType = static_cast<RenderOperation::OperationType>(batchIt->operationType);
		renderSystem->_render(m_renderOperation);
		m_renderOperation.vertexData->vertexStart += batchIt->vertexCount;
	}

	m_sprites.clear();
}

void TwoDManager::spriteBltFull(
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
)
{
	TexturePtr tex;
	Sprite item;
	item.x1 = x0;
	item.y1 = y0;
	item.x2 = x1;
	item.y2 = y1;
	item.tx1 = u0;
	item.ty1 = v0;
	item.tx2 = u1;
	item.ty2 = v1;
	RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();
	renderSystem->convertColourValue(ColourValue(color.r, color.g, color.b, color.a), &item.colour);
	item.operationType = 4;
	item.segment = 0;
	if (texture != "") {
		tex = TextureManager::getSingleton().getByName(texture);
		if (tex.isNull()) {
			tex = TextureManager::getSingleton().load(texture, "General");
		}
		if (tex.isNull()) {
			item.texHandle = 0;
		}
		else {
			item.texHandle = tex->getHandle();
			if (x0 == x1 && y0 == y1) {
				x1 = x0 + tex->getWidth();
				y1 = y0 + tex->getHeight();
			}
		}
	}
	else {
		item.texHandle = 0;
	}
	m_sprites.push_back(item);
}

void TwoDManager::rectBlt(int type, float x0, float y0, float x1, float y1, const G3D::Color4& color)
{
	Sprite item;
	item.x1 = x0;
	item.y1 = y0;
	item.x2 = x1;
	item.y2 = y1;
	Root::getSingleton().getRenderSystem()->convertColourValue(
		ColourValue(color.r, color.g, color.b, color.a), &item.colour
	);
	item.texHandle = 0;
	item.operationType = type;
	item.segment = 0;
	m_sprites.push_back(item);
}

void TwoDManager::lineBlt(float x0, float y0, float x1, float y1, const G3D::Color4& color)
{
	Sprite item;
	item.x1 = x0;
	item.y1 = y0;
	item.x2 = x1;
	item.y2 = y1;
	Root::getSingleton().getRenderSystem()->convertColourValue(
		ColourValue(color.r, color.g, color.b, color.a), &item.colour
	);
	item.texHandle = 0;
	item.operationType = 2;
	item.segment = 1;
	m_sprites.push_back(item);
}

void TwoDManager::renderQueueStarted(uint8 queueGroupId, const String& invocation, bool& skipThisInvocation)
{
	if (!m_afterQueue && queueGroupId == m_targetQueue) {
		renderBuffer();
	}
}

void TwoDManager::renderQueueEnded(uint8 queueGroupId, const String& invocation, bool& repeatThisInvocation)
{
	if (m_afterQueue && queueGroupId == m_targetQueue) {
		renderBuffer();
	}
}

} // namespace Ogre
